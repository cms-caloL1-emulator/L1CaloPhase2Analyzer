/*
 * Description:
 *   Implementation of the all-stage GCT link analyzer.
 *
 *   The analyzer preserves every 576-bit word exactly as eighteen 32-bit
 *   chunks, and decodes only formats that are unambiguous at a single-link
 *   boundary.  IP1 links 0..8 are kept raw because their semantic word stream
 *   is assembled by several specialized EG/PF packing helpers.
 *
 *   For PreIP2 ST links the producer performs the required IP1->IP2 48-bit
 *   slot transpose.  st_slot_transposed is recorded so firmware comparisons
 *   can distinguish raw PostIP1 packing from the actual attached-IP2 input.
 */

#include "L1Trigger/L1CaloPhase2Analyzer/interface/L1TCaloAnalyzer.h"

#include <array>
#include <utility>

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"

namespace {
constexpr int kRawChunks = 18;
constexpr int kObjects64 = 9;
constexpr int kObjects48 = 12;

// Same Figure-6 pair-label table as the producer.  This is a combinatorial
// route table, not proof that the current RCT collection pair index equals the
// physical hardware card-pair label; see SOURCE_BASIS_AND_LIMITS.md in the delivered overlay.
constexpr int kRegionPairMap[6][4] = {
    {11, 0, 1, 2}, {1, 2, 3, 4}, {3, 4, 5, 6},
    {5, 6, 7, 8}, {7, 8, 9, 10}, {9, 10, 11, 0}};
}

L1TCaloAnalyzer::L1TCaloAnalyzer(const edm::ParameterSet& config) {
  usesResource("TFileService");

  const std::array<std::string, 6> preIP1Names{{
      "GCT1SLR3PreIP1", "GCT1SLR1PreIP1", "GCT2SLR3PreIP1",
      "GCT2SLR1PreIP1", "GCT3SLR3PreIP1", "GCT3SLR1PreIP1"}};
  const std::array<std::string, 6> postIP1Names{{
      "GCT1SLR3PostIP1", "GCT1SLR1PostIP1", "GCT2SLR3PostIP1",
      "GCT2SLR1PostIP1", "GCT3SLR3PostIP1", "GCT3SLR1PostIP1"}};
  const std::array<std::string, 3> preIP2Names{{"GCT1PreIP2", "GCT2PreIP2", "GCT3PreIP2"}};
  const std::array<std::string, 3> postIP2Names{{"GCT1PostIP2", "GCT2PostIP2", "GCT3PostIP2"}};

  for (std::size_t index = 0; index < preIP1Names.size(); ++index) {
    const auto& name = preIP1Names[index];
    preIP1Sources_.push_back(
        {consumes<RCTCollection>(config.getParameter<edm::InputTag>(name)), name, static_cast<int>(index)});
  }
  for (std::size_t index = 0; index < postIP1Names.size(); ++index) {
    const auto& name = postIP1Names[index];
    gctSources_.push_back(
        {consumes<GCTCollection>(config.getParameter<edm::InputTag>(name)), "PostIP1", name, static_cast<int>(index)});
  }
  for (std::size_t index = 0; index < preIP2Names.size(); ++index) {
    const auto& name = preIP2Names[index];
    gctSources_.push_back(
        {consumes<GCTCollection>(config.getParameter<edm::InputTag>(name)), "PreIP2", name, static_cast<int>(index)});
  }
  for (std::size_t index = 0; index < postIP2Names.size(); ++index) {
    const auto& name = postIP2Names[index];
    gctSources_.push_back(
        {consumes<GCTCollection>(config.getParameter<edm::InputTag>(name)), "PostIP2", name, static_cast<int>(index)});
  }
}

void L1TCaloAnalyzer::beginJob() {
  linkTree_ = fileService_->make<TTree>("linkTree", "GCT links at PreIP1, PostIP1, PreIP2, and PostIP2");

  linkTree_->Branch("run", &run_);
  linkTree_->Branch("lumi", &lumi_);
  linkTree_->Branch("event", &event_);
  linkTree_->Branch("stage", &stage_);
  linkTree_->Branch("region_name", &regionName_);
  linkTree_->Branch("region_index", &regionIndex_);
  linkTree_->Branch("link_index", &linkIndex_);
  linkTree_->Branch("link_type", &linkType_);
  linkTree_->Branch("phi_slot", &phiSlot_);
  linkTree_->Branch("eta_side", &etaSide_);
  linkTree_->Branch("ip1_region_slot", &ip1RegionSlot_);
  linkTree_->Branch("local_link_index", &localLinkIndex_);
  linkTree_->Branch("rct_pair_index", &rctPairIndex_);
  linkTree_->Branch("rct_collection_index", &rctCollectionIndex_);
  linkTree_->Branch("route_source", &routeSource_);
  linkTree_->Branch("route_source_link", &routeSourceLink_);
  linkTree_->Branch("st_slot_transposed", &stSlotTransposed_);
  linkTree_->Branch("raw_word32", &rawWord32_);

  linkTree_->Branch("object_type", &objectType_);
  linkTree_->Branch("object_index", &objectIndex_);
  linkTree_->Branch("energy", &energy_);
  linkTree_->Branch("em_energy", &emEnergy_);
  linkTree_->Branch("seed_energy", &seedEnergy_);
  linkTree_->Branch("eta", &eta_);
  linkTree_->Branch("phi", &phi_);
  linkTree_->Branch("hoe", &hoe_);
  linkTree_->Branch("flags", &flags_);
  linkTree_->Branch("ratio", &ratio_);
  linkTree_->Branch("et5x5", &et5x5_);
  linkTree_->Branch("quality", &quality_);
  linkTree_->Branch("timing", &timing_);
  linkTree_->Branch("brems", &brems_);
  linkTree_->Branch("ex", &ex_);
  linkTree_->Branch("ey", &ey_);
  linkTree_->Branch("ht", &ht_);
}

void L1TCaloAnalyzer::analyze(const edm::Event& event, const edm::EventSetup&) {
  run_ = event.id().run();
  lumi_ = event.id().luminosityBlock();
  event_ = event.id().event();

  for (const auto& source : preIP1Sources_) {
    const auto handle = event.getHandle(source.token);
    if (!handle.isValid()) {
      edm::LogWarning("L1TCaloAnalyzer") << "Missing analyzer input " << source.name;
      continue;
    }
    fillPreIP1Collection(*handle, source);
  }

  for (const auto& source : gctSources_) {
    const auto handle = event.getHandle(source.token);
    if (!handle.isValid()) {
      edm::LogWarning("L1TCaloAnalyzer") << "Missing analyzer input " << source.name;
      continue;
    }
    fillGCTCollection(*handle, source);
  }
}

void L1TCaloAnalyzer::fillPreIP1Collection(const RCTCollection& collection, const RCTSource& source) {
  for (std::size_t link = 0; link < collection.size(); ++link) {
    const LinkWord word = collection[link].data();
    prepareLink("PreIP1", source.name, source.regionIndex, static_cast<int>(link), word);
    decodePreIP1(word, static_cast<int>(link));
    linkTree_->Fill();
  }
}

void L1TCaloAnalyzer::fillGCTCollection(const GCTCollection& collection, const GCTSource& source) {
  for (std::size_t link = 0; link < collection.size(); ++link) {
    const LinkWord word = collection[link].data();
    prepareLink(source.stage, source.name, source.regionIndex, static_cast<int>(link), word);

    if (source.stage == "PostIP1") {
      decodePostIP1(word, static_cast<int>(link));
    } else if (source.stage == "PreIP2") {
      decodePreIP2(word, static_cast<int>(link));
    } else if (source.stage == "PostIP2") {
      decodePostIP2(word, static_cast<int>(link));
    }
    linkTree_->Fill();
  }
}

void L1TCaloAnalyzer::prepareLink(const std::string& stage,
                                  const std::string& regionName,
                                  int regionIndex,
                                  int linkIndex,
                                  const LinkWord& word) {
  stage_ = stage;
  regionName_ = regionName;
  regionIndex_ = regionIndex;
  linkIndex_ = linkIndex;
  linkType_.clear();
  phiSlot_ = -1;
  etaSide_ = 0;
  ip1RegionSlot_ = -1;
  localLinkIndex_ = -1;
  rctPairIndex_ = -1;
  rctCollectionIndex_ = -1;
  routeSource_.clear();
  routeSourceLink_ = -1;
  stSlotTransposed_ = false;

  rawWord32_.clear();
  objectType_.clear();
  objectIndex_.clear();
  energy_.clear();
  emEnergy_.clear();
  seedEnergy_.clear();
  eta_.clear();
  phi_.clear();
  hoe_.clear();
  flags_.clear();
  ratio_.clear();
  et5x5_.clear();
  quality_.clear();
  timing_.clear();
  brems_.clear();
  ex_.clear();
  ey_.clear();
  ht_.clear();

  rawWord32_.reserve(kRawChunks);
  for (int chunk = 0; chunk < kRawChunks; ++chunk) {
    const int first = 32 * chunk;
    rawWord32_.push_back(static_cast<std::uint32_t>(word.range(first + 31, first)));
  }
}

void L1TCaloAnalyzer::decodePreIP1(const LinkWord& word, int linkIndex) {
  // The producer groups the four positive-eta RCT cards first, followed by the
  // four negative-eta cards in reverse PDF pair-label order.  This is not a
  // validated physical-detector order until the upstream RCT phi partition is
  // aligned to Figure 6.  Each card occupies
  // four consecutive links: cluster, tower0, tower1, tower2.
  ip1RegionSlot_ = linkIndex / 4;
  localLinkIndex_ = linkIndex % 4;
  etaSide_ = (ip1RegionSlot_ < 4) ? +1 : -1;
  phiSlot_ = (etaSide_ > 0) ? ip1RegionSlot_ : 7 - ip1RegionSlot_;

  if (regionIndex_ >= 0 && regionIndex_ < 6 && phiSlot_ >= 0 && phiSlot_ < 4) {
    rctPairIndex_ = kRegionPairMap[regionIndex_][phiSlot_];
    rctCollectionIndex_ = 2 * rctPairIndex_ + ((etaSide_ > 0) ? 1 : 0);
  }

  if (localLinkIndex_ == 0) {
    linkType_ = "rct_cluster";
    for (int index = 0; index < kObjects64; ++index) {
      const int start = 64 * index;
      const int seed = static_cast<int>(word.range(start + 9, start));
      const int energy = static_cast<int>(word.range(start + 21, start + 10));
      const int eta = static_cast<int>(word.range(start + 28, start + 22));
      const int phi = static_cast<int>(word.range(start + 33, start + 29));
      const int et5x5 = static_cast<int>(word.range(start + 43, start + 34));
      const int quality = static_cast<int>(word.range(start + 50, start + 44));
      const int timing = static_cast<int>(word.range(start + 55, start + 51));
      const int flags = static_cast<int>(word.range(start + 57, start + 56));
      const int brems = static_cast<int>(word.range(start + 59, start + 58));
      appendObject("rct_cluster", index, energy, -1, seed, eta, phi, -1, flags, -1, et5x5, quality, timing, brems);
    }
    return;
  }

  linkType_ = "rct_tower";
  const int phiBase = 2 * (localLinkIndex_ - 1);
  for (int etaIndex = 0; etaIndex < 17; ++etaIndex) {
    for (int phiOffset = 0; phiOffset < 2; ++phiOffset) {
      const int start = etaIndex * 16 + phiOffset * 272;
      const int energy = static_cast<int>(word.range(start + 9, start));
      const int hoe = static_cast<int>(word.range(start + 13, start + 10));
      const int flags = static_cast<int>(word.range(start + 15, start + 14));
      const int objectIndex = 2 * etaIndex + phiOffset;
      appendObject("rct_tower", objectIndex, energy, -1, -1, etaIndex, phiBase + phiOffset, hoe, flags);
    }
  }
}

void L1TCaloAnalyzer::decodePostIP1(const LinkWord& word, int linkIndex) {
  if (linkIndex <= 8) {
    // Preserve the exact word.  The nine 576-bit containers jointly represent
    // the 81-word IP1 CL1 payload assembled by specialized EG/PF packers; a
    // raw representation avoids assigning a wrong semantic type to a slot.
    linkType_ = "ip1_mixed_eg_pf";
    return;
  }

  if (linkIndex <= 12) {
    linkType_ = "ip1_eg";
    for (int index = 0; index < kObjects64; ++index) {
      const int start = 64 * index;
      const int energy = static_cast<int>(word.range(start + 11, start));
      // Dedicated IP1-to-IP2 EG links carry local, unsigned RCT/GCT
      // coordinates in the current implementation.  Do not sign-extend them;
      // values eta>63 would otherwise be displayed as negative.
      const int eta = static_cast<int>(word.range(start + 18, start + 12));
      const int phi = static_cast<int>(word.range(start + 25, start + 19));
      const int hoe = static_cast<int>(word.range(start + 29, start + 26));
      const int quality = static_cast<int>(word.range(start + 42, start + 30));
      const int timing = static_cast<int>(word.range(start + 47, start + 43));
      const int brems = static_cast<int>(word.range(start + 51, start + 50));
      appendObject("ip1_eg", index, energy, -1, -1, eta, phi, hoe, -1, -1, -1, quality, timing, brems);
    }
    return;
  }

  if (linkIndex <= 20) {
    linkType_ = "ip1_stower";
    for (int index = 0; index < kObjects48; ++index) {
      const int start = 48 * index;
      appendObject("ip1_stower",
                   index,
                   static_cast<int>(word.range(start + 11, start)),
                   static_cast<int>(word.range(start + 23, start + 12)),
                   -1,
                   static_cast<int>(word.range(start + 27, start + 24)),
                   static_cast<int>(word.range(start + 32, start + 28)),
                   -1,
                   static_cast<int>(word.range(start + 47, start + 33)));
    }
    return;
  }

  linkType_ = "unexpected_post_ip1_link";
}

void L1TCaloAnalyzer::decodePreIP2(const LinkWord& word, int linkIndex) {
  routeSource_ = preIP2RouteSource(linkIndex);
  routeSourceLink_ = preIP2RouteLink(linkIndex);

  if (linkIndex >= 0 && linkIndex <= 7) {
    linkType_ = "ip2_eg_input";
    // Each input word contains eight 64-bit IP1 EG payloads.
    for (int index = 0; index < 8; ++index) {
      const int start = 64 * index;
      appendObject("ip2_eg_input",
                   index,
                   static_cast<int>(word.range(start + 11, start)),
                   -1,
                   -1,
                   static_cast<int>(word.range(start + 18, start + 12)),
                   static_cast<int>(word.range(start + 25, start + 19)));
    }
    return;
  }

  const bool isSTowerInput = (linkIndex >= 8 && linkIndex <= 10) ||
                             (linkIndex >= 13 && linkIndex <= 15) ||
                             (linkIndex >= 17 && linkIndex <= 22);
  if (isSTowerInput) {
    linkType_ = "ip2_stower_input";
    stSlotTransposed_ = true;
    for (int index = 0; index < kObjects48; ++index) {
      const int start = 48 * index;
      appendObject("ip2_stower_input",
                   index,
                   static_cast<int>(word.range(start + 11, start)),
                   static_cast<int>(word.range(start + 23, start + 12)),
                   -1,
                   static_cast<int>(word.range(start + 27, start + 24)),
                   static_cast<int>(word.range(start + 32, start + 28)),
                   -1,
                   static_cast<int>(word.range(start + 47, start + 33)));
    }
    return;
  }

  linkType_ = "ip2_unused_input";
}

void L1TCaloAnalyzer::decodePostIP2(const LinkWord& word, int linkIndex) {
  if (linkIndex == 0 || linkIndex == 3) {
    linkType_ = "ip2_eg_output";
    for (int index = 0; index < kObjects48; ++index) {
      const int start = 48 * index;
      appendObject("ip2_eg",
                   index,
                   static_cast<int>(word.range(start + 11, start)),
                   -1,
                   -1,
                   static_cast<int>(word.range(start + 18, start + 12)),
                   static_cast<int>(word.range(start + 25, start + 19)));
    }
    return;
  }

  if (linkIndex == 1 || linkIndex == 4) {
    linkType_ = "ip2_jet_tau_output";
    for (int index = 0; index < kObjects48; ++index) {
      const int start = 48 * index;
      const std::string type = (index < 6) ? "ip2_jet" : "ip2_tau";
      appendObject(type,
                   index % 6,
                   static_cast<int>(word.range(start + 11, start)),
                   -1,
                   -1,
                   signExtend(static_cast<unsigned int>(word.range(start + 17, start + 12)), 6),
                   signExtend(static_cast<unsigned int>(word.range(start + 24, start + 18)), 7),
                   -1,
                   -1,
                   static_cast<int>(word.range(start + 30, start + 27)));
    }
    return;
  }

  if (linkIndex == 2) {
    linkType_ = "ip2_sums_output";
    appendObject("ip2_sums",
                 0,
                 -1,
                 -1,
                 -1,
                 -999,
                 -999,
                 -1,
                 -1,
                 -1,
                 -1,
                 -1,
                 -1,
                 -1,
                 static_cast<int>(word.range(11, 0)),
                 static_cast<int>(word.range(23, 12)),
                 static_cast<int>(word.range(35, 24)));
    return;
  }

  linkType_ = "ip2_spare_output";
}

void L1TCaloAnalyzer::appendObject(const std::string& type,
                                   int index,
                                   int energy,
                                   int emEnergy,
                                   int seedEnergy,
                                   int eta,
                                   int phi,
                                   int hoe,
                                   int flags,
                                   int ratio,
                                   int et5x5,
                                   int quality,
                                   int timing,
                                   int brems,
                                   int ex,
                                   int ey,
                                   int ht) {
  objectType_.push_back(type);
  objectIndex_.push_back(index);
  energy_.push_back(energy);
  emEnergy_.push_back(emEnergy);
  seedEnergy_.push_back(seedEnergy);
  eta_.push_back(eta);
  phi_.push_back(phi);
  hoe_.push_back(hoe);
  flags_.push_back(flags);
  ratio_.push_back(ratio);
  et5x5_.push_back(et5x5);
  quality_.push_back(quality);
  timing_.push_back(timing);
  brems_.push_back(brems);
  ex_.push_back(ex);
  ey_.push_back(ey);
  ht_.push_back(ht);
}

int L1TCaloAnalyzer::signExtend(unsigned int value, unsigned int width) {
  const unsigned int signBit = 1U << (width - 1U);
  const unsigned int mask = (1U << width) - 1U;
  value &= mask;
  return static_cast<int>((value ^ signBit) - signBit);
}

std::string L1TCaloAnalyzer::preIP2RouteSource(int linkIndex) {
  if ((linkIndex >= 0 && linkIndex <= 3) || (linkIndex >= 8 && linkIndex <= 10) ||
      (linkIndex >= 13 && linkIndex <= 15)) {
    return "SLR3";
  }
  if ((linkIndex >= 4 && linkIndex <= 7) || (linkIndex >= 17 && linkIndex <= 22)) {
    return "SLR1";
  }
  return "zero";
}

int L1TCaloAnalyzer::preIP2RouteLink(int linkIndex) {
  static constexpr std::array<int, 24> kSourceLinks{{
      9, 10, 11, 12, 9, 10, 11, 12,
      13, 14, 15, -1, -1, 18, 19, 20,
      -1, 14, 15, 16, 17, 18, 19, -1}};
  return (linkIndex >= 0 && linkIndex < static_cast<int>(kSourceLinks.size())) ? kSourceLinks[linkIndex] : -1;
}

void L1TCaloAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription description;
  const std::array<std::string, 18> names{{
      "GCT1SLR3PreIP1", "GCT1SLR1PreIP1", "GCT2SLR3PreIP1", "GCT2SLR1PreIP1",
      "GCT3SLR3PreIP1", "GCT3SLR1PreIP1", "GCT1SLR3PostIP1", "GCT1SLR1PostIP1",
      "GCT2SLR3PostIP1", "GCT2SLR1PostIP1", "GCT3SLR3PostIP1", "GCT3SLR1PostIP1",
      "GCT1PreIP2", "GCT2PreIP2", "GCT3PreIP2", "GCT1PostIP2", "GCT2PostIP2", "GCT3PostIP2"}};
  for (const auto& name : names) {
    description.add<edm::InputTag>(name, edm::InputTag("l1tPhase2GCTEmulatorProducer", name));
  }
  descriptions.add("l1TCaloAnalyzer", description);
}

DEFINE_FWK_MODULE(L1TCaloAnalyzer);

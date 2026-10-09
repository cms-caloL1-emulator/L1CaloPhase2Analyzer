/*
 *  \file L1TCaloAnalyzer.cc
 *  Author R. Simeon
 */

#include "L1Trigger/L1CaloPhase2Analyzer/interface/L1TCaloAnalyzer.h"

#include <array>
#include <utility>

#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/HcalDetId/interface/HcalSubdetector.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"
#include "DataFormats/L1THGCal/interface/HGCalTower.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"

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

#include "L1Trigger/L1CaloPhase2Analyzer/interface/L1TCaloAnalyzer.h"
#include "DataFormats/Math/interface/deltaR.h"


// ECAL propagation
#include "CommonTools/BaseParticlePropagator/interface/BaseParticlePropagator.h"
#include "CommonTools/BaseParticlePropagator/interface/RawParticle.h"

//////////////////////////////////////////////////////////////////////////////////////

using namespace edm;

namespace p2CaloAnalyzer {

L1TCaloAnalyzer::L1TCaloAnalyzer( const ParameterSet & cfg ) :
{
  const auto outputSumsLinks = cfg.getParameter<std::vector<edm::InputTag>>("outputSumsLinks");
  const auto outputGTLinks = cfg.getParameter<std::vector<edm::InputTag>>("outputGTLinks");
  for (unsigned int i = 0; i < p2CaloAnalyzer::kOutputLinks; ++i){
    outputSumsLinkTokens_[i] = consumes<std::vector<uint64_t>>(outputSumsLinks[i]);
    outputGTLinkTokens_[i] = consumes<std::vector<uint64_t>>(outputGTLinks[i]);
  }

  gctsTree = tfs_->make<TTree>("gctsTree", "GCTSum Output Tree");

  gctsTree->Branch("run",    &run,     "run/I");
  gctsTree->Branch("lumi",   &lumi,    "lumi/I");
  gctsTree->Branch("event",  &event,   "event/I");
  
  ////putting bufsize at 32000 and changing split level to 0 so that the branch isn't split into multiple branches

  for (unsigned int i = 0; i < p2CaloAnalyzer::kOutputLinks; ++i){
    gctsTree->Branch(std::string("linkOutSums") + std::to_string(i), "vector<int>", &(linkOutSums[i]), 32000, 0);
    gctsTree->Branch(std::string("linkOutGT") + std::to_string(i), "vector<int>", &(linkOutGT[i]), 32000, 0);
  }

}

void L1TCaloAnalyzer::beginJob( const EventSetup & es) {
}

void L1TCaloAnalyzer::analyze( const Event& evt, const EventSetup& es )
 {

  run = evt.id().run();
  lumi = evt.id().luminosityBlock();
  event = evt.id().event();

  for (unsigned int i = 0; i < p2CaloAnalyzer::kOutputLinks; ++i){
    linkOutSums[i]->clear();
    linkOutGT[i]->clear();

    if(evt.getByToken(outputSumsLinkTokens_[i], outputSumsLinkHandles_[i])){
      for (unsigned int word = 0; word < p2CaloAnalyzer::kWordsPerLink; ++word) {
        linkOutSums[i]->push_back((*outputSumsLinkHandles_[i])[word]);
      }
    }
    if(evt.getByToken(outputGTLinkTokens_[i], outputGTLinkHandles_[i])){
      for (unsigned int word = 0; word < p2CaloAnalyzer::kWordsPerLink; ++word) {
        linkOutGT[i]->push_back((*outputGTLinkHandles_[i])[word]);
      }
    }
    return;
  }

  gctsTree->Fill();
 
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

}

DEFINE_FWK_MODULE(p2CaloAnalyzer::L1TCaloAnalyzer);

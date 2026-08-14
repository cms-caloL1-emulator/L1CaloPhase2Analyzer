/*
 * Description:
 *   ROOT analyzer for every link boundary in the Phase-2 GCT emulator chain.
 *
 *   The analyzer reads all six PreIP1 collections, all six PostIP1
 *   collections, all three routed PreIP2 collections, and all three PostIP2
 *   collections.  It writes one TTree entry per 576-bit link.  Every entry
 *   contains all eighteen 32-bit raw chunks plus decoded object vectors when
 *   the link has an unambiguous firmware layout.
 *
 *   Keeping one entry per link makes the output convenient for link-by-link
 *   firmware comparison while preserving the exact raw payload at every
 *   processing boundary.
 */

#ifndef L1Trigger_L1CaloPhase2Analyzer_L1TCaloAnalyzer_h
#define L1Trigger_L1CaloPhase2Analyzer_L1TCaloAnalyzer_h

#include <ap_int.h>

#include <cstdint>
#include <string>
#include <vector>

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/GCT_output.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/RCT_output.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "TTree.h"

class L1TCaloAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit L1TCaloAnalyzer(const edm::ParameterSet&);
  ~L1TCaloAnalyzer() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions&);

private:
  using RCTCollection = l1tp2::rctOutputLinkCollection;
  using GCTCollection = l1tp2::gctOutputLinkCollection;
  using LinkWord = ap_uint<576>;

  struct RCTSource {
    edm::EDGetTokenT<RCTCollection> token;
    std::string name;
    int regionIndex;
  };

  struct GCTSource {
    edm::EDGetTokenT<GCTCollection> token;
    std::string stage;
    std::string name;
    int regionIndex;
  };

  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;

  void fillPreIP1Collection(const RCTCollection&, const RCTSource&);
  void fillGCTCollection(const GCTCollection&, const GCTSource&);
  void prepareLink(const std::string& stage,
                   const std::string& regionName,
                   int regionIndex,
                   int linkIndex,
                   const LinkWord& word);
  void decodePreIP1(const LinkWord& word, int linkIndex);
  void decodePostIP1(const LinkWord& word, int linkIndex);
  void decodePreIP2(const LinkWord& word, int linkIndex);
  void decodePostIP2(const LinkWord& word, int linkIndex);

  void appendObject(const std::string& type,
                    int index,
                    int energy = -1,
                    int emEnergy = -1,
                    int seedEnergy = -1,
                    int eta = -999,
                    int phi = -999,
                    int hoe = -1,
                    int flags = -1,
                    int ratio = -1,
                    int et5x5 = -1,
                    int quality = -1,
                    int timing = -1,
                    int brems = -1,
                    int ex = -1,
                    int ey = -1,
                    int ht = -1);

  static int signExtend(unsigned int value, unsigned int width);
  static std::string preIP2RouteSource(int linkIndex);
  static int preIP2RouteLink(int linkIndex);

  std::vector<RCTSource> preIP1Sources_;
  std::vector<GCTSource> gctSources_;

  edm::Service<TFileService> fileService_;
  TTree* linkTree_{nullptr};

  // Event and link identity.
  std::uint32_t run_{0};
  std::uint32_t lumi_{0};
  std::uint64_t event_{0};
  std::string stage_;
  std::string regionName_;
  int regionIndex_{-1};
  int linkIndex_{-1};
  std::string linkType_;

  // Metadata specific to routed/pre-IP1 links.
  int phiSlot_{-1};
  int etaSide_{0};
  int ip1RegionSlot_{-1};
  int localLinkIndex_{-1};
  int rctPairIndex_{-1};
  int rctCollectionIndex_{-1};
  std::string routeSource_;
  int routeSourceLink_{-1};
  bool stSlotTransposed_{false};

  // Raw payload: 18 x 32 bits = 576 bits.
  std::vector<std::uint32_t> rawWord32_;

  // Aligned decoded-object vectors.  Unused fields are filled with sentinels.
  std::vector<std::string> objectType_;
  std::vector<int> objectIndex_;
  std::vector<int> energy_;
  std::vector<int> emEnergy_;
  std::vector<int> seedEnergy_;
  std::vector<int> eta_;
  std::vector<int> phi_;
  std::vector<int> hoe_;
  std::vector<int> flags_;
  std::vector<int> ratio_;
  std::vector<int> et5x5_;
  std::vector<int> quality_;
  std::vector<int> timing_;
  std::vector<int> brems_;
  std::vector<int> ex_;
  std::vector<int> ey_;
  std::vector<int> ht_;
};

#endif

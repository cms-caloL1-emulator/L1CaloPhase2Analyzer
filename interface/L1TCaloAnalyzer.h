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

namespace p2CaloAnalyzer {

static constexpr int kOutputLinks = 6;
static constexpr int kWordsPerLink = 9;

class L1TCaloAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {

 public:
  
  // Constructor
  L1TCaloAnalyzer(const edm::ParameterSet& ps);
  
  // Destructor
  virtual ~L1TCaloAnalyzer();

  edm::Service<TFileService> tfs_;

  std::vector<uint64_t>* *linkOutSums = new std::vector<uint64_t>[kOutputLinks];
  std::vector<uint64_t>* *linkOutGT = new std::vector<uint64_t>[kOutputLinks];

  TTree* gctsTree;

  int run, lumi, event;

 protected:
  // Analyze
  void analyze(const edm::Event& evt, const edm::EventSetup& es);
  
  // BeginJob
  void beginJob(const edm::EventSetup &es);
  
  // EndJob
  void endJob(void);

  
 private:
  // ----------member data ---------------------------

  std::array<edm::EDGetTokenT<std::vector<uint64_t>>, kOutputLinks> outputSumsLinkTokens_;
  std::array<edm::EDGetTokenT<std::vector<uint64_t>>, kOutputLinks> outputGTLinkTokens_;

  std::array<edm::Handle<std::vector<uint64_t>>, kOutputLinks> outputSumsLinkHandles_;
  std::array<edm::Handle<std::vector<uint64_t>>, kOutputLinks> outputGTLinkHandles_;

};

void getIP3OutputClusters(
  ap_uint<576> Data,
  std::vector<int>* RCT_seed_pt,
  std::vector<int>* RCT_pt,
  std::vector<int>* RCT_eta,
  std::vector<int>* RCT_phi,
  std::vector<int>* RCT_et5x5,
  std::vector<int>* RCT_wps,
  std::vector<int>* RCT_timing,
  std::vector<int>* RCT_spike,
  std::vector<int>* RCT_satur,
  std::vector<int>* RCT_brems,
  std::vector<int>* RCT_spare
);

void getIP3OutputTowers(
  ap_uint<576> Data,
  int whichLink,
  std::vector<int>* RCT_et,
  std::vector<int>* RCT_eta,
  std::vector<int>* RCT_phi,
  std::vector<int>* RCT_hoe,
  std::vector<int>* RCT_fb
);

} // namespace p2CaloAnalyzer

#endif

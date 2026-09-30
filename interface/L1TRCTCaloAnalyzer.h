#ifndef L1TCaloAnalyzer_H
#define L1TCaloAnalyzer_H


// system include files
#include <memory>
#include <unistd.h>


#include <iostream>
#include <fstream>
#include <vector>

#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TH2F.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
//#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"

// GCT and RCT data formats
#include "DataFormats/L1CaloTrigger/interface/L1CaloCollections.h"
#include "DataFormats/L1GlobalCaloTrigger/interface/L1GctCollections.h"
#include "DataFormats/TauReco/interface/PFTau.h"
#include "DataFormats/TauReco/interface/PFTauDiscriminator.h"
#include "DataFormats/L1Trigger/interface/L1JetParticle.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/L1THGCal/interface/HGCalTower.h"

#include "DataFormats/TauReco/interface/BaseTau.h"
#include "DataFormats/TauReco/interface/PFTauFwd.h"
#include "DataFormats/TauReco/interface/PFTauTagInfo.h"

#include <memory>
#include <math.h>
#include <vector>
#include <list>
#include <TLorentzVector.h>

#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "CondFormats/L1TObjects/interface/L1CaloHcalScale.h"
#include "CondFormats/DataRecord/interface/L1CaloHcalScaleRcd.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "L1Trigger/L1TGlobal/interface/TriggerMenuFwd.h"
#include "DataFormats/L1Trigger/interface/Tau.h"
#include "DataFormats/L1CaloTrigger/interface/L1CaloRegion.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/CaloCrystalCluster.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/CaloPFCluster.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/Phase2L1CaloJet.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/DigitizedL1CaloJet.h"
#include "DataFormats/L1THGCal/interface/HGCalTower.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/RCT_output.h"

#ifdef __MAKECINT__
// #pragma extra_include "TLorentzVector.h";
#pragma link C++ class std::vector<TLorentzVector>;
#endif

//
// class declaration
//
using std::vector;

class L1TCaloAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {

 public:
  
  // Constructor
  L1TCaloAnalyzer(const edm::ParameterSet& ps);
  
  // Destructor
  virtual ~L1TCaloAnalyzer();

  edm::Service<TFileService> tfs_;

  std::vector<int> *linkOut0 = new std::vector<int>;
  std::vector<int> *linkOut1 = new std::vector<int>;
  std::vector<int> *linkOut2 = new std::vector<int>;
  std::vector<int> *linkOut3 = new std::vector<int>;

  // Vectors of vectors, where each element is a per-card vector
  // Cluster branches
  std::vector<std::vector<int>> *seed_pt = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *cluster_pt = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *cluster_eta = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *cluster_phi = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *et5x5 = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *wps = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *timing = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *spike = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *satur = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *brems = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *spare = new std::vector<std::vector<int>>;
  // Tower branches
  std::vector<std::vector<int>> *tower_et = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *tower_eta = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *tower_phi = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *hoe = new std::vector<std::vector<int>>;
  std::vector<std::vector<int>> *fb = new std::vector<std::vector<int>>;

  // Per-card vectors (clusters)
  std::vector<int> *RCT_seed_pt = new std::vector<int>;
  std::vector<int> *RCT_cluster_pt = new std::vector<int>;
  std::vector<int> *RCT_cluster_eta = new std::vector<int>;
  std::vector<int> *RCT_cluster_phi = new std::vector<int>;
  std::vector<int> *RCT_et5x5 = new std::vector<int>;
  std::vector<int> *RCT_wps = new std::vector<int>;
  std::vector<int> *RCT_timing = new std::vector<int>;
  std::vector<int> *RCT_spike = new std::vector<int>;
  std::vector<int> *RCT_satur = new std::vector<int>;
  std::vector<int> *RCT_brems = new std::vector<int>;
  std::vector<int> *RCT_spare = new std::vector<int>;

  // Per-card vectors (towers)
  std::vector<int> *RCT_tower_et = new std::vector<int>;
  std::vector<int> *RCT_tower_eta = new std::vector<int>;
  std::vector<int> *RCT_tower_phi = new std::vector<int>;
  std::vector<int> *RCT_hoe = new std::vector<int>;
  std::vector<int> *RCT_fb = new std::vector<int>;

  TTree* linkTree;

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

  edm::ESGetToken<CaloTPGTranscoder, CaloTPGRecord> decoderToken_;

  edm::ESGetToken<CaloGeometry, CaloGeometryRecord> caloGeometryToken_;
  edm::ESGetToken<HcalTopology, HcalRecNumberingRecord> hbTopologyToken_;

  edm::EDGetTokenT<l1tp2::rctOutputLinkCollection> link0Src_;
  edm::EDGetTokenT<l1tp2::rctOutputLinkCollection> link1Src_;
  edm::EDGetTokenT<l1tp2::rctOutputLinkCollection> link2Src_;
  edm::EDGetTokenT<l1tp2::rctOutputLinkCollection> link3Src_;

  std::string folderName_;

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

#endif

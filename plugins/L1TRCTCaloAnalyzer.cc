/*
 *  \file L1TCaloAnalyzer.cc
 *  Authors S. Kwan, P. Das, I. Ojalvo, R. Simeon
 */

// system include files
#include <ap_int.h>
#include <array>
#include <cmath>
// #include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <TLorentzVector.h>
#ifdef __MAKECINT__
#pragma link C++ class vector<TLorentzVector>+;
#endif

// user include files
#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "CalibFormats/CaloTPG/interface/CaloTPGTranscoder.h"
#include "CalibFormats/CaloTPG/interface/CaloTPGRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/EcalAlgo/interface/EcalBarrelGeometry.h"
#include "Geometry/HcalTowerAlgo/interface/HcalTrigTowerGeometry.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/HcalDetId/interface/HcalSubdetector.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"
#include "DataFormats/L1THGCal/interface/HGCalTower.h"
#include "DataFormats/HcalDigi/interface/HcalDigiCollections.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/RCT_output.h"


// ECAL TPs
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"

// HCAL TPs
#include "DataFormats/HcalDigi/interface/HcalTriggerPrimitiveDigi.h"

// Output tower collection
#include "DataFormats/L1TCalorimeterPhase2/interface/CaloCrystalCluster.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/CaloTower.h"
#include "DataFormats/L1TCalorimeterPhase2/interface/CaloPFCluster.h"
#include "DataFormats/L1Trigger/interface/EGamma.h"

#include "L1Trigger/L1CaloTrigger/interface/ParametricCalibration.h"
#include "L1Trigger/L1TCalorimeter/interface/CaloTools.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "L1Trigger/L1CaloPhase2Analyzer/interface/L1TCaloAnalyzer.h"
#include "DataFormats/Math/interface/deltaR.h"


// ECAL propagation
#include "CommonTools/BaseParticlePropagator/interface/BaseParticlePropagator.h"
#include "CommonTools/BaseParticlePropagator/interface/RawParticle.h"

//////////////////////////////////////////////////////////////////////////////////////

using namespace edm;

L1TCaloAnalyzer::L1TCaloAnalyzer( const ParameterSet & cfg ) :
  decoderToken_(esConsumes<CaloTPGTranscoder, CaloTPGRecord>(edm::ESInputTag("", ""))),
  caloGeometryToken_(esConsumes<CaloGeometry, CaloGeometryRecord>(edm::ESInputTag("", ""))),
  hbTopologyToken_(esConsumes<HcalTopology, HcalRecNumberingRecord>(edm::ESInputTag("", ""))),
  link0Src_(consumes<l1tp2::rctOutputLinkCollection>(cfg.getParameter<edm::InputTag>("LinkOut0"))),
  link1Src_(consumes<l1tp2::rctOutputLinkCollection>(cfg.getParameter<edm::InputTag>("LinkOut1"))),
  link2Src_(consumes<l1tp2::rctOutputLinkCollection>(cfg.getParameter<edm::InputTag>("LinkOut2"))),
  link3Src_(consumes<l1tp2::rctOutputLinkCollection>(cfg.getParameter<edm::InputTag>("LinkOut3")))
{
    folderName_          = cfg.getUntrackedParameter<std::string>("folderName");

    linkTree = tfs_->make<TTree>("linkTree", "RCT Output Link Tree");

    linkTree->Branch("run",    &run,     "run/I");
    linkTree->Branch("lumi",   &lumi,    "lumi/I");
    linkTree->Branch("event",  &event,   "event/I");
    
    ////putting bufsize at 32000 and changing split level to 0 so that the branch isn't split into multiple branches

    linkTree->Branch("linkOut0", "vector<int>", &linkOut0, 32000, 0);
    linkTree->Branch("linkOut1", "vector<int>", &linkOut1, 32000, 0);
    linkTree->Branch("linkOut2", "vector<int>", &linkOut2, 32000, 0);
    linkTree->Branch("linkOut3", "vector<int>", &linkOut3, 32000, 0);

    // Cluster branches
    linkTree->Branch("cluster_seed_pt", "vector<vector<int>>", &seed_pt, 32000, 0);
    linkTree->Branch("cluster_pt", "vector<vector<int>>", &cluster_pt, 32000, 0);
    linkTree->Branch("cluster_eta", "vector<vector<int>>", &cluster_eta, 32000, 0);
    linkTree->Branch("cluster_phi", "vector<vector<int>>", &cluster_phi, 32000, 0);
    linkTree->Branch("cluster_et5x5", "vector<vector<int>>", &et5x5, 32000, 0);
    linkTree->Branch("cluster_wps", "vector<vector<int>>", &wps, 32000, 0);
    linkTree->Branch("cluster_timing", "vector<vector<int>>", &timing, 32000, 0);
    linkTree->Branch("cluster_spike", "vector<vector<int>>", &spike, 32000, 0);
    linkTree->Branch("cluster_satur", "vector<vector<int>>", &satur, 32000, 0);
    linkTree->Branch("cluster_brems", "vector<vector<int>>", &brems, 32000, 0);
    linkTree->Branch("cluster_spare", "vector<vector<int>>", &spare, 32000, 0);

    // Tower branches
    linkTree->Branch("tower_et", "vector<vector<int>>", &tower_et, 32000, 0);
    linkTree->Branch("tower_eta", "vector<vector<int>>", &tower_eta, 32000, 0);
    linkTree->Branch("tower_phi", "vector<vector<int>>", &tower_phi, 32000, 0);
    linkTree->Branch("tower_hoe", "vector<vector<int>>", &hoe, 32000, 0);
    linkTree->Branch("tower_fb", "vector<vector<int>>", &fb, 32000, 0);

  }

void L1TCaloAnalyzer::beginJob( const EventSetup & es) {
}

void L1TCaloAnalyzer::analyze( const Event& evt, const EventSetup& es )
 {

  run = evt.id().run();
  lumi = evt.id().luminosityBlock();
  event = evt.id().event();

  edm::Handle<l1tp2::rctOutputLinkCollection> linkOut0_int576;
  edm::Handle<l1tp2::rctOutputLinkCollection> linkOut1_int576;
  edm::Handle<l1tp2::rctOutputLinkCollection> linkOut2_int576;
  edm::Handle<l1tp2::rctOutputLinkCollection> linkOut3_int576;

  linkOut0->clear();
  linkOut1->clear();
  linkOut2->clear();
  linkOut3->clear();

  // Clear cluster vectors
  seed_pt->clear();
  cluster_pt->clear();
  cluster_eta->clear();
  cluster_phi->clear();
  et5x5->clear();
  wps->clear();
  timing->clear();
  spike->clear();
  satur->clear();
  brems->clear();
  spare->clear();

  // Clear tower vectors
  tower_et->clear();
  tower_eta->clear();
  tower_phi->clear();
  hoe->clear();
  fb->clear();

  if(evt.getByToken(link0Src_, linkOut0_int576)){
    for(const auto & link : *linkOut0_int576){
      linkOut0->push_back((int)link.data());

      getIP3OutputClusters(
        link.data(),
        RCT_seed_pt,
        RCT_cluster_pt,
        RCT_cluster_eta,
        RCT_cluster_phi,
        RCT_et5x5,
        RCT_wps,
        RCT_timing,
        RCT_spike,
        RCT_satur,
        RCT_brems,
        RCT_spare
      );
      seed_pt->push_back(*RCT_seed_pt);
      cluster_pt->push_back(*RCT_cluster_pt);
      cluster_eta->push_back(*RCT_cluster_eta);
      cluster_phi->push_back(*RCT_cluster_phi);
      et5x5->push_back(*RCT_et5x5);
      wps->push_back(*RCT_wps);
      timing->push_back(*RCT_timing);
      spike->push_back(*RCT_spike);
      satur->push_back(*RCT_satur);
      brems->push_back(*RCT_brems);
      spare->push_back(*RCT_spare);
    }
  }

  if(evt.getByToken(link1Src_, linkOut1_int576)){
    for(const auto & link : *linkOut1_int576){
      linkOut1->push_back((int)link.data());

      getIP3OutputTowers(
        link.data(),
        1,
        RCT_tower_et,
        RCT_tower_eta,
        RCT_tower_phi,
        RCT_hoe,
        RCT_fb
      );
      tower_et->push_back(*RCT_tower_et);
      tower_eta->push_back(*RCT_tower_eta);
      tower_phi->push_back(*RCT_tower_phi);
      hoe->push_back(*RCT_hoe);
      fb->push_back(*RCT_fb);
    }
  }

  if(evt.getByToken(link2Src_, linkOut2_int576)){
    for(const auto & link : *linkOut2_int576){
      linkOut2->push_back((int)link.data());

      getIP3OutputTowers(
        link.data(),
        2,
        RCT_tower_et,
        RCT_tower_eta,
        RCT_tower_phi,
        RCT_hoe,
        RCT_fb
      );
      tower_et->push_back(*RCT_tower_et);
      tower_eta->push_back(*RCT_tower_eta);
      tower_phi->push_back(*RCT_tower_phi);
      hoe->push_back(*RCT_hoe);
      fb->push_back(*RCT_fb);
    }
  }

  if(evt.getByToken(link3Src_, linkOut3_int576)){
    for(const auto & link : *linkOut3_int576){
      linkOut3->push_back((int)link.data());

      getIP3OutputTowers(
        link.data(),
        3,
        RCT_tower_et,
        RCT_tower_eta,
        RCT_tower_phi,
        RCT_hoe,
        RCT_fb
      );
      tower_et->push_back(*RCT_tower_et);
      tower_eta->push_back(*RCT_tower_eta);
      tower_phi->push_back(*RCT_tower_phi);
      hoe->push_back(*RCT_hoe);
      fb->push_back(*RCT_fb);
    }
  }

  linkTree->Fill();
 
 }


void L1TCaloAnalyzer::endJob() {
}

L1TCaloAnalyzer::~L1TCaloAnalyzer(){
}

//////////////////////////// Utility functions ///////////////////////////////
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
) {
  
  RCT_seed_pt->clear();
  RCT_pt->clear();
  RCT_eta->clear();
  RCT_phi->clear();
  RCT_et5x5->clear();
  RCT_wps->clear();
  RCT_timing->clear();
  RCT_spike->clear();
  RCT_satur->clear();
  RCT_brems->clear();
  RCT_spare->clear();

  for(int i=0; i<9; i++){
    int start = i*64;
    
    int this_seed_pt = (int)Data.range(start+9,start);
    RCT_seed_pt->push_back(this_seed_pt);
    int this_pt = (int)Data.range(start+21,start+10);
    RCT_pt->push_back(this_pt);
    int this_eta = (int)Data.range(start+28,start+22);
    RCT_eta->push_back(this_eta);
    int this_phi = (int)Data.range(start+33,start+29);
    RCT_phi->push_back(this_phi);
    int this_et5x5 = (int)Data.range(start+43,start+34);
    RCT_et5x5->push_back(this_et5x5);
    int this_wps = (int)Data.range(start+50,start+44);
    RCT_wps->push_back(this_wps);
    int this_timing = (int)Data.range(start+55,start+51);
    RCT_timing->push_back(this_timing);
    int this_spike = (int)Data.range(start+56,start+56);
    RCT_spare->push_back(this_spike);
    int this_satur = (int)Data.range(start+57,start+57);
    RCT_spare->push_back(this_satur);
    int this_brems = (int)Data.range(start+59,start+58);
    RCT_brems->push_back(this_brems);
    int this_spare = (int)Data.range(start+63,start+60);
    RCT_spare->push_back(this_spare);
  }

}

void getIP3OutputTowers(
  ap_uint<576> Data,
  int whichLink,
  std::vector<int>* RCT_et,
  std::vector<int>* RCT_eta,
  std::vector<int>* RCT_phi,
  std::vector<int>* RCT_hoe,
  std::vector<int>* RCT_fb
) {

  int this_et;
  int this_hoe;
  int this_fb;

  RCT_et->clear();
  RCT_eta->clear();
  RCT_phi->clear();
  RCT_hoe->clear();
  RCT_fb->clear();

  for(int i=0; i<17; i++) {
    // Lower iPhi in this link
    int this_phi = (whichLink - 1)*2;
    int start = i*16;

    this_et = (int)Data.range(start+9,start);
    RCT_et->push_back(this_et);
    RCT_eta->push_back(i);
    RCT_phi->push_back(this_phi);
    this_hoe = (int)Data.range(start+13,start+10);
    RCT_hoe->push_back(this_hoe);
    this_fb = (int)Data.range(start+15,start+14);
    RCT_fb->push_back(this_fb);

    // Higher iPhi in this link
    this_phi = (whichLink - 1)*2 + 1;
    start = i*16 + 272;
    
    this_et = (int)Data.range(start+9,start);
    RCT_et->push_back(this_et);
    RCT_eta->push_back(i);
    RCT_phi->push_back(this_phi);
    this_hoe = (int)Data.range(start+13,start+10);
    RCT_hoe->push_back(this_hoe);
    this_fb = (int)Data.range(start+15,start+14);
    RCT_fb->push_back(this_fb);
  }

}

DEFINE_FWK_MODULE(L1TCaloAnalyzer);

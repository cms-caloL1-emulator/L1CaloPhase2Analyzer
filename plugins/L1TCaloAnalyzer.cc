/*
 *  \file L1TCaloAnalyzer.cc
 *  Author R. Simeon
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
  }

  gctsTree->Fill();
 
 }


void L1TCaloAnalyzer::endJob() {
}

L1TCaloAnalyzer::~L1TCaloAnalyzer(){
}

}

DEFINE_FWK_MODULE(L1TCaloAnalyzer);

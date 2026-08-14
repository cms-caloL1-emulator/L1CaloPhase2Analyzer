"""Description:
Configure the link-level analyzer for all four GCT processing boundaries:
PreIP1, PostIP1, routed PreIP2, and PostIP2.
"""

import FWCore.ParameterSet.Config as cms

l1NtupleProducer = cms.EDAnalyzer(
    "L1TCaloAnalyzer",
    GCT1SLR3PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1SLR3PreIP1"),
    GCT1SLR1PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1SLR1PreIP1"),
    GCT2SLR3PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2SLR3PreIP1"),
    GCT2SLR1PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2SLR1PreIP1"),
    GCT3SLR3PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3SLR3PreIP1"),
    GCT3SLR1PreIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3SLR1PreIP1"),
    GCT1SLR3PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1SLR3PostIP1"),
    GCT1SLR1PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1SLR1PostIP1"),
    GCT2SLR3PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2SLR3PostIP1"),
    GCT2SLR1PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2SLR1PostIP1"),
    GCT3SLR3PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3SLR3PostIP1"),
    GCT3SLR1PostIP1=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3SLR1PostIP1"),
    GCT1PreIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1PreIP2"),
    GCT2PreIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2PreIP2"),
    GCT3PreIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3PreIP2"),
    GCT1PostIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT1PostIP2"),
    GCT2PostIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT2PostIP2"),
    GCT3PostIP2=cms.InputTag("l1tPhase2GCTEmulatorProducer", "GCT3PostIP2"),
)

import FWCore.ParameterSet.Config as cms

from Configuration.StandardSequences.Eras import eras

process = cms.Process("L1AlgoTest",eras.Phase2C17I13M9)

process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.Geometry.GeometryExtendedRun4D110Reco_cff')
process.load('Configuration.Geometry.GeometryExtendedRun4D110_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.SimL1Emulator_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1) )

process.source = cms.Source("PoolSource",
  fileNames = cms.untracked.vstring(
    "file:///hdfs/store/user/rsimeon/MCFiles/001ebf5f-b83c-43fc-997f-c2e5ecf1f9dd.root",
  ),
  inputCommands = cms.untracked.vstring(
    "keep *",
    "drop l1tTkPrimaryVertexs_*_*_*",
  )
)

# Global Tag :
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '131X_mcRun4_realistic_v6', '')

# Add HCAL Transcoder
process.load('SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff')
process.load('CalibCalorimetry.CaloTPG.CaloTPGTranscoder_cfi')

# Run L1 simulation (necessary to get HGCal towers)
process.load("L1Trigger.L1CaloTrigger.l1tPhase2RCTEmulatorProducer_cfi")
process.L1simulation_step = cms.Path(process.l1tPhase2RCTEmulatorProducer)

# Run the analyzer
process.load('L1Trigger.L1CaloPhase2Analyzer.l1TCaloAnalyzer_cfi')

process.RCT = cms.Path( process.l1tPhase2RCTEmulatorProducer*process.l1NtupleProducer )

# output file
process.TFileService = cms.Service("TFileService",
    fileName = cms.string('analyzer.root')
)

process.schedule = cms.Schedule(process.RCT)

# Multi-threading
process.options.numberOfThreads=cms.untracked.uint32(8)
process.options.numberOfStreams=cms.untracked.uint32(0)

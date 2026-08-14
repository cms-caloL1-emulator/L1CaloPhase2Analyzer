"""
Description
-----------
Run the complete Phase-2 barrel calorimeter chain:

    Input DIGI/RAW
        -> RCT
        -> GCT IP1
        -> GCT IP2
        -> link-level analyzer

The analyzer saves the GCT link payload at:

    1. PreIP1
    2. PostIP1
    3. PreIP2
    4. PostIP2

Default input sample:
    Phase2HLTTDRWinter20DIGI
    DoubleElectron_FlatPt-1To100
    NoPU

By default all events in all listed files are processed.
"""

import FWCore.ParameterSet.Config as cms

from Configuration.AlCa.GlobalTag import GlobalTag
from Configuration.StandardSequences.Eras import eras
from FWCore.ParameterSet.VarParsing import VarParsing


# =============================================================================
# Command-line options
# =============================================================================

# VarParsing("analysis") already provides:
#   inputFiles, outputFile, maxEvents, secondaryInputFiles
#
# Therefore do NOT register these names manually.
options = VarParsing("analysis")

# Default output ROOT file.
options.setDefault("outputFile", "gct_ip1_ip2_links.root")

# -1 means process every event available in the input files.
options.setDefault("maxEvents", -1)

# Parse the command line exactly once.
options.parseArguments()


# =============================================================================
# Default input files
# =============================================================================

defaultInputFiles = [
    "file:///hdfs/store/user/rsimeon/MCFiles/001ebf5f-b83c-43fc-997f-c2e5ecf1f9dd.root",
]


# If inputFiles=... was supplied on the cmsRun command line,
# use that instead of the default files above.
if not options.inputFiles:
    options.inputFiles = defaultInputFiles


# =============================================================================
# Process
# =============================================================================

process = cms.Process(
    "L1AlgoTest",
    eras.Phase2C17I13M9,
)


# =============================================================================
# Standard CMSSW services / conditions
# =============================================================================

process.load("Configuration.StandardSequences.Services_cff")
process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("Configuration.EventContent.EventContent_cff")
process.load("SimGeneral.MixingModule.mixNoPU_cfi")

process.load("Configuration.Geometry.GeometryExtendedRun4D110Reco_cff")
process.load("Configuration.Geometry.GeometryExtendedRun4D110_cff")

process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("Configuration.StandardSequences.SimL1Emulator_cff")
process.load("Configuration.StandardSequences.EndOfProcess_cff")
process.load(
    "Configuration.StandardSequences.FrontierConditions_GlobalTag_cff"
)

process.load("SimCalorimetry.HcalTrigPrimProducers.hcaltpdigi_cff")
process.load("CalibCalorimetry.CaloTPG.CaloTPGTranscoder_cfi")


# Keep the same GlobalTag used by the existing project test configuration.
process.GlobalTag = GlobalTag(
    process.GlobalTag,
    "140X_mcRun4_realistic_v4",
    "",
)


# =============================================================================
# Event source
# =============================================================================

process.maxEvents = cms.untracked.PSet(
    input=cms.untracked.int32(options.maxEvents)
)

process.source = cms.Source(
    "PoolSource",

    fileNames=cms.untracked.vstring(
        options.inputFiles
    ),

    inputCommands=cms.untracked.vstring(
        "keep *",
        "drop l1tTkPrimaryVertexs_*_*_*",
    ),
)


# =============================================================================
# Load the emulator chain
# =============================================================================

# RCT:
#
#   ECAL/HCAL TP
#       ->
#   RCT LinkOut0..3
#
process.load(
    "L1Trigger.L1CaloTrigger.l1tPhase2RCTEmulatorProducer_cfi"
)


# GCT:
#
#   RCT
#      ->
#   PreIP1
#      ->
#   IP1
#      ->
#   PostIP1
#      ->
#   PreIP2
#      ->
#   IP2
#      ->
#   PostIP2
#
process.load(
    "L1Trigger.L1CaloTrigger.l1tPhase2GCTEmulatorProducer_cfi"
)


# Analyzer:
#
#   saves:
#       PreIP1
#       PostIP1
#       PreIP2
#       PostIP2
#
process.load(
    "L1Trigger.L1CaloPhase2Analyzer.l1TCaloAnalyzer_cfi"
)


# =============================================================================
# Logging
# =============================================================================

# For a short debugging run, reportEvery = 1 is useful.
#
# For a large/all-event run, I would increase this to 100 or 1000.
process.MessageLogger.cerr.FwkReport.reportEvery = 1


# =============================================================================
# Analyzer ROOT output
# =============================================================================

process.TFileService = cms.Service(
    "TFileService",
    fileName=cms.string(options.outputFile),
)


# =============================================================================
# Processing path
# =============================================================================

process.RCTGCTAnalyzer = cms.Path(

    process.l1tPhase2RCTEmulatorProducer

    * process.l1tPhase2GCTEmulatorProducer

    * process.l1NtupleProducer
)


process.schedule = cms.Schedule(
    process.RCTGCTAnalyzer
)
import FWCore.ParameterSet.Config as cms

l1LinkProducer = cms.EDAnalyzer("L1TCaloAnalyzer",
                                  outputSumsLinks = cms.VInputTag(
                                    *[cms.InputTag("l1tPhase2L1GCTSumEmulator", f"LinkOut{i}") for i in range(6)]
                                    ),
                                  outputGTLinks = cms.VInputTag(
                                    *[cms.InputTag("l1tPhase2L1GCTSumEmulator", f"SumLinkOut{i}") for i in range(6)]
                                  )
)

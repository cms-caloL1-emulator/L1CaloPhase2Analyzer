"""Description:
Configure the link-level analyzer for all four GCT processing boundaries:
PreIP1, PostIP1, routed PreIP2, and PostIP2.
"""

l1LinkProducer = cms.EDAnalyzer("L1TCaloAnalyzer",
                                  outputSumsLinks = cms.VInputTag(
                                    *[cms.InputTag("l1tPhase2L1GCTSumEmulator", f"LinkOut{i}") for i in range(6)]
                                    ),
                                  outputGTLinks = cms.VInputTag(
                                    *[cms.InputTag("l1tPhase2L1GCTSumEmulator", f"SumLinkOut{i}") for i in range(6)]
                                  )
)

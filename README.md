# Phase 2 L1 Calo Analyzer

## Description

   Forked from https://github.com/pallabidas/L1CaloPhase2Analyzer.git, branch 13_3_0_calojet, which in turn
   is adapted from: https://github.com/skkwan/phase2-l1Calo-analyzer.
   This repo is for running the Phase-2 calo emulator, in particular comparing the new, firmware-based emulator
   that is also digitized against the old emulator.

## Prerequisites to Running

   CMSSW_15_0_0_pre3 is meant to be run with an el8 OS. If you are running on a non-el8 machine, you can
   use an el8 container. For example, one can do the following. Go to the project root directory (the
   same directory as the one in which you will run `cmsrel`), and create a file called `.bashrc` with the
   following contents:
   ```
   unset SCRAM_ARCH
   source /cvmfs/cms.cern.ch/cmsset_default.sh
   ```
   and then run the following commands in the command line:
   ```
   export IMAGE=cms:rhel8-x86_64

   singularity exec -B ${PWD}:/srv -B /etc/condor -B /etc/grid-security/certificates \
   -B /cvmfs -B /hdfs -B /scratch -B /afs --pwd /srv /cvmfs/singularity.opensciencegrid.org/cmssw/${IMAGE} \
   /bin/bash --rcfile /srv/.bashrc
   ```

   It is recommended to put the last code block into a file called `shell`. Then, before proceeding, run
   `./shell` in the command line. This must be done every time before using the
   code in this repository. Note that you can run `exit` to exit the container.

## Setup (do only once)

   ```
   cmsrel CMSSW_15_0_0_pre3
   cd CMSSW_15_0_0_pre3/src
   cmsenv
   git cms-init
   git cms-addpkg L1Trigger/L1TCalorimeter
   git cms-addpkg L1Trigger/L1CaloTrigger
   git cms-addpkg DataFormats/L1TCalorimeterPhase2
   cd L1Trigger
   git clone https://github.com/rpsimeon34/L1CaloPhase2Analyzer.git -b 15_0_0_pre3_calojet
   cd ../
   git remote add digi-repo https://github.com/rpsimeon34/cmssw.git
   git fetch digi-repo
   ```
   The remaining lines depend on whether you want to run the RCT emulator or the ML on RCT setup producer.

   For RCT emulator:
   ```
   git checkout -b digi-branch --track digi-repo/from-CMSSW_15_0_0_pre3
   git pull
   scram b -j 12
   ```

   For ML on RCT setup producer:
   ```
   git checkout -b digi-branch --track digi-repo/from-CMSSW_15_0_0_pre3-MLonAPXsetup
   git pull
   scram b -j 12
   ```

## To run the RCT emulator and create ntuples

   For getting the ntuple of the new emulator's cluster outputs:
   ```
   cd L1Trigger/L1CaloPhase2Analyzer/test/
   cmsRun test-analyzer.py
   ```
   This runs the RCT-only `L1TRCTAnalyzer` and writes the event-level branches used by
   `compare_old_new.py`. For GCT PreIP1/PostIP1/PreIP2/PostIP2 link output, run
   `test_gct_chain.py`, which uses the separate `L1TCaloAnalyzer` plugin.
   To get the ntuple of the old emulator's cluster outputs (in the same directory):
   ```
   cmsRun test-old-analyzer.py
   ```
   To generate plots showing the difference between the clusters from the new and old emulators, run the
   following. Note that this script requires packages not included by default in the above-named container
   image:
   ```
   python3 compare_old_new.py
   ```

## To run the ML on RCT setup producer and create feature ntuples

   To get ntuples corresponding to the features for the ML on RCT project:
   ```
   cd L1Trigger/L1CaloPhase2Analyzer/test/
   cmsRun run-MLonRCTsetup.py
   ```

## Examining the ML on RCT feature ntuples

   This repository also contains a script that allows you to examine the ntuple created by the previous
   step. It is in the same directory as `run-MLonRCTsetup.py`, and is called `examine_MLonRCT_features.py`.
   This script creates displays of clusters', ECAL towers', and HCAL towers' energies.
   It has the following configurable parameters, set as constants at the top of the file:

   - Which event from the feature ntuple to examine
   - Which RCT card to examine
   - Path to the feature ntuple
   - Where to save the generated plots

   Note that you must create the directory in which you want to save plots (the one specified in the last
   configurable parameter listed above). It can then be run with
   ```
   python3 examine_MLonRCT_features.py
   ```

## Contents of the ML on RCT feature ntuples

   Cluster information is separated out by which SLR that cluster comes from. For example, there is a branch
   `SLR3_cluster_energy`, `SLR2_cluster_energy`, `SLR1_cluster_energy`, and `SLR0_cluster_energy`.
   Each branch is a 3D array. The outermost axis corresponds to iEvent. The middle axis corresponds to which
   RCT card the information is from (so the length on this axis is 24 - one entry for each card). The innermost
   axis runs over the clusters. For each SLR, the following cluster branches exist:
   
   - Seed energy
   - Energy
   - i $\eta$ (in RCT coordinates)
   - i $\phi$ (in RCT coordinates)
   - $E_T$ 5x5
   - $E_T$ 2x5
   - Timing
   - Spike
   - Saturation
   - Brems

   ECAL tower information is also separated out by SLR. Again, each branch is a 3D array. The outermost axis is
   events, the middle axis is RCT card, and the innermost axis runs over the towers. For each SLR, the following
   ECAL tower branches exist:

   - Energy
   - i $\eta$ (in RCT coordinates)
   - i $\phi$ (in RCT coordinates)
   - Timing
   - Spike

   HCAL tower information is separated out by which input link (== which BCP card) the tower comes from.
   **Importantly, this means that for a given RCT card, its corresponding array will contain towers that do not
   actually belong to that RCT card's part of the detector.** These towers will have i $\phi$ either < 0 or
   $\geq$ 6 (since each RCT card is 6 towers wide in $\phi$). The branches are labeled by which input link
   the information comes from. For example, there is a branch `HCAL8_tower_et`, `HCAL7_tower_et`, `HCAL6_tower_et`,
   and `HCAL5_tower_et`. Each branch is a 3D array. The outermost axis is events, the middle axis is RCT card, and
   the innermost axis runs over the towers. For each input link, the following HCAL tower branches exist:

   - Energy
   - i $\eta$ (in RCT coordinates)
   - i $\phi$ (in RCT coordinates - so values < 0 or $\geq$ 6 are towers outside this RCT card)
   - Feature bits

   The HCAL feature bits are given here as integers, but are to be understood as 6 bits. For example, if a tower has
   a feature bits value of 37, this should be understood as 100101. Here, we call the most significant bit "Bit 0",
   and the least significant bit "Bit 5". Bits 0 through 3 are as described in Section 6 of
   (the HCAL timing trigger detector note)[https://cds.cern.ch/record/2891496/files/DN2023_022.pdf].

   - Bit 0: Depth flag. Is significant energy deposited in layers 3+, but not in layers 1 and 2? Set if there is < 1 GeV
     in each of depths 1 and 2, and $\geq$ 5 GeV in at least one of the depths 3+.
   - Bit 1: Prompt timing flag. Is there a "prompt" cell within the tower at or above 4 GeV?
   - Bit 2: Delay 1 timing flag. Is there a "delayed" cell within the tower at or above 4 GeV?
   - Bit 3: Delay 2 timing flag. Is there a "very delayed" cell within the tower at or above 4 GeV?
   - Bit 4: Is the first layer consistent with a MIP?
   - Bit 5: Is the first layer consistent with a MIP **and** is there at least one layer with more than MIP energy deposition?

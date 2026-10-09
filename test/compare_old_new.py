import awkward as ak
import matplotlib.pyplot as plt
from hist import axis, Hist
import hist.plot
import mplhep as hep
import numpy as np
from pathlib import Path
import uproot

NEW_PATH = "analyzer.root"
OLD_PATH = "old_analyzer.root"
PLOT_DIR = "plots/"

TOWER_DIFF_MIN_ENERGY = 0.5 #min. energy to compare towers

ECAL_ETA_RANGE = 1.4841
HCAL_ETA_RANGE = 1.3968
CRYSTAL_SIZE_ETA = ECAL_ETA_RANGE/85 #85 crystals per card
CRYSTAL_SIZE_PHI = np.pi / 180 #12 cards cover 2*pi, 30 crystals per card
TOWER_SIZE_ETA = ECAL_ETA_RANGE/17
TOWER_SIZE_PHI = np.pi / 36
PHI_CARD_LENGTH = np.pi / 6
TOWERS_ETA_PER_CARD = 17
TOWERS_PHI_PER_CARD = 6

######## Utility functions

def convert_to_eta(iEta,n_cards=24):
    for cc in range(n_cards):
        sign = 2*(cc % 2) - 1 #if cc odd, positive eta

        these_iEta = iEta[:,cc,:]
        these_eta = sign*CRYSTAL_SIZE_ETA*(0.5 + these_iEta)

        these_eta_wrapped = ak.singletons(these_eta,axis=0)
        if cc == 0:
            out = these_eta_wrapped
        else:
            out = ak.concatenate((out,these_eta_wrapped),axis=1)

    return out

def convert_to_phi(iPhi,n_cards=24):
    for cc in range(n_cards):
        #if cc even, iPhi 0 is further in phi
        sign = 2*(cc % 2) - 1
        if cc % 2 == 0:
            offset = -np.pi + (cc//2 + 1) * PHI_CARD_LENGTH
        else:
            offset = -np.pi + cc//2 * PHI_CARD_LENGTH

        these_iPhi = iPhi[:,cc,:]
        these_phi = offset + sign*CRYSTAL_SIZE_PHI*(0.5 + these_iPhi)

        these_phi_wrapped = ak.singletons(these_phi,axis=0)
        if cc == 0:
            out = these_phi_wrapped
        else:
            out = ak.concatenate((out,these_phi_wrapped),axis=1)
    
    return out

def convert_tower_global_iEta(iEta,n_cards=24):
    '''
    iEta expected to have shape nEvents * (n_cards*3) * (TOWERS_ETA_PER_CARD*2).
    The *3 is because there are 3 links per card, each of which is its own vector
    in the L1TCaloAnalyzer. The *2 is because there are 2 iPhi's worth of towers
    per link.
    '''
    cc = ak.local_index(iEta,axis=1) % n_cards #which card are we on?
    out = ak.where(
        cc % 2 == 1,
        TOWERS_ETA_PER_CARD + iEta, #odd card
        TOWERS_ETA_PER_CARD - 1 - iEta #even card
    )
    return out

def convert_tower_global_iPhi(iPhi,n_cards=24):
    '''
    iPhi expected to have shape nEvents * (n_cards*3) * (TOWERS_ETA_PER_CARD*2).
    The *3 is because there are 3 links per card, each of which is its own vector
    in the L1TCaloAnalyzer. The *2 is because there are 2 iPhi's worth of towers
    per link.
    '''
    cc = ak.local_index(iPhi,axis=1) % n_cards #which card are we on?
    out = ak.where(
        cc % 2 == 1,
        ((cc // 2) * TOWERS_PHI_PER_CARD) + iPhi, #odd card
        ((cc // 2) * TOWERS_PHI_PER_CARD) + (TOWERS_PHI_PER_CARD - 1 - iPhi) #even card
    )
    return out

def get_new_tree():
    with uproot.open(NEW_PATH) as f:
        tree = f["l1NtupleProducer/linkTree;1"]
    return tree

def get_old_tree():
    with uproot.open(OLD_PATH) as f:
        tree = f["l1NtupleOldProducer/outputTree;1"]
    return tree

######### Cluster functions

def top2_clusters(pt,eta,phi):
    sorted_inds = ak.argsort(pt,ascending=False)
    top2_inds = sorted_inds[:,:2]
    return pt[top2_inds], eta[top2_inds], phi[top2_inds]

def compare_clusters(old_tree,new_tree):
    new_pt = new_tree["cluster_pt"].array()*0.5 #account for LSB
    new_eta = new_tree["cluster_eta"].array()
    new_phi = new_tree["cluster_phi"].array()
    old_pt = old_tree["cluster_pt"].array()
    old_eta = old_tree["cluster_eta"].array()
    old_phi = old_tree["cluster_phi"].array()

    #Convert new emulator coordinates from relative to card to global
    new_eta = convert_to_eta(new_eta)
    new_phi = convert_to_phi(new_phi)

    #Flatten new emulator clusters to look at all cards at once
    new_pt = ak.flatten(new_pt,axis=2)
    new_eta = ak.flatten(new_eta,axis=2)
    new_phi = ak.flatten(new_phi,axis=2)

    #Get top 2 clusters
    new_pt, new_eta, new_phi = top2_clusters(new_pt,new_eta,new_phi)
    old_pt, old_eta, old_phi = top2_clusters(old_pt,old_eta,old_phi)

    print("Old clusters (in eta,phi):")
    print(f"    {old_pt[:,0]} at {old_eta[:,0]} , {old_phi[:,0]}")
    print(f"    {old_pt[:,1]} at {old_eta[:,1]} , {old_phi[:,1]}")

    print("New clusters (in eta,phi):")
    print(f"    {new_pt[:,0]} at {new_eta[:,0]} , {new_phi[:,0]}")
    print(f"    {new_pt[:,1]} at {new_eta[:,1]} , {new_phi[:,1]}")

    new_filt = new_pt > 0
    new_pt = new_pt[new_filt]
    new_eta = new_eta[new_filt]
    new_phi = new_phi[new_filt]
    old_filt = old_pt > 0
    old_pt = old_pt[old_filt]
    old_eta = old_eta[old_filt]
    old_phi = old_phi[old_filt]

    pt_diff = new_pt - old_pt
    eta_diff = new_eta - old_eta
    phi_diff = abs(new_phi - old_phi)

    return pt_diff/old_pt, eta_diff, ak.where(phi_diff < np.pi, phi_diff, 2*np.pi - phi_diff)

def get_new_top2_clusters(tree):
    new_pt = tree["cluster_pt"].array()*0.5 #account for LSB
    new_eta = tree["cluster_eta"].array()
    new_phi = tree["cluster_phi"].array()

    #Convert new emulator coordinates from relative to card to global
    new_eta = convert_to_eta(new_eta)
    new_phi = convert_to_phi(new_phi)

    #Flatten new emulator clusters to look at all cards at once
    new_pt = ak.flatten(new_pt,axis=2)
    new_eta = ak.flatten(new_eta,axis=2)
    new_phi = ak.flatten(new_phi,axis=2)

    #Get top 2 clusters
    return top2_clusters(new_pt,new_eta,new_phi)

def get_old_top2_clusters(tree):
    old_pt = tree["cluster_pt"].array()
    old_eta = tree["cluster_eta"].array()
    old_phi = tree["cluster_phi"].array()

    return top2_clusters(old_pt,old_eta,old_phi)

######## Tower functions

def compare_towers(old_tree,new_tree):
    new_et = new_tree["tower_et"].array()*0.5 #Account for LSB
    new_eta = new_tree["tower_eta"].array()
    new_phi = new_tree["tower_phi"].array()
    new_hoe = new_tree["tower_hoe"].array()
    old_ecal_et = old_tree["tower_ecal_et"].array()
    old_hcal_et = old_tree["tower_hcal_et"].array()
    old_eta = old_tree["tower_eta"].array()
    old_phi = old_tree["tower_phi"].array()
    old_et = old_ecal_et + old_hcal_et
    old_hoe = old_hcal_et/old_et

    #Eliminate towers with energy below X GeV
    new_filt = new_et >= TOWER_DIFF_MIN_ENERGY
    new_et = new_et[new_filt]
    new_eta = new_eta[new_filt]
    new_phi = new_phi[new_filt]
    new_hoe = new_hoe[new_filt]
    old_filt = old_et >= TOWER_DIFF_MIN_ENERGY
    old_et = old_et[old_filt]
    old_eta = old_eta[old_filt]
    old_phi = old_phi[old_filt]
    old_hoe = old_hoe[old_filt]

    #Convert new emulator coordinates from relative to card to global
    new_eta = convert_tower_global_iEta(new_eta)
    new_phi = convert_tower_global_iPhi(new_phi)

    #Flatten new emulator towers to look at all cards at once
    new_et = ak.flatten(new_et,axis=2)
    new_eta = ak.flatten(new_eta,axis=2)
    new_phi = ak.flatten(new_phi,axis=2)
    new_hoe = ak.flatten(new_hoe,axis=2)

    #Match towers by location between old and new emulator
    eta_pairs = ak.unzip(ak.cartesian((old_eta,new_eta))) #eta_pairs[0] is old
    phi_pairs = ak.unzip(ak.cartesian((old_phi,new_phi)))
    #Require old and new emulator agree on eta and phi
    filt = ((eta_pairs[0] == eta_pairs[1]) & (phi_pairs[0] == phi_pairs[1]))
    et_pairs = ak.unzip(ak.cartesian((old_et,new_et)))
    hoe_pairs = ak.unzip(ak.cartesian((old_hoe,new_hoe)))
    all_et_rel_diffs = (et_pairs[1] - et_pairs[0])/et_pairs[0]
    all_et_diffs = et_pairs[1] - et_pairs[0]
    all_hoe_diffs = (hoe_pairs[1] - hoe_pairs[0])
    #Only consider diffs when old and new tower have identical eta and phi
    et_rel_diffs = all_et_rel_diffs[filt]
    et_diffs = all_et_diffs[filt]
    hoe_diffs = all_hoe_diffs[filt]

    return et_rel_diffs, et_diffs, hoe_diffs

def get_new_et_hoe(new_tree):
    et = new_tree["tower_et"].array()*0.5 #Account for LSB
    hoe = new_tree["tower_hoe"].array()
    return ak.ravel(et), ak.ravel(hoe)

def get_old_et_hoe(old_tree):
    ecal_et = old_tree["tower_ecal_et"].array()
    hcal_et = old_tree["tower_hcal_et"].array()
    et = ecal_et + hcal_et
    et = et
    hcal_et = hcal_et
    hoe = hcal_et/et
    return ak.ravel(et), ak.ravel(hoe)

def plot_clusters(tree,is_new,iEvent=0,max_et=None):
    cluster_pt = tree["cluster_pt"].array()
    if is_new:
        cluster_pt = cluster_pt*0.5 #account for LSB
    cluster_eta = tree["cluster_eta"].array()
    cluster_phi = tree["cluster_phi"].array()

    if is_new:
        cluster_eta = convert_to_eta(cluster_eta)
        cluster_phi = convert_to_phi(cluster_phi)

    cluster_pt = cluster_pt[iEvent]
    cluster_eta = cluster_eta[iEvent]
    cluster_phi = cluster_phi[iEvent]

    #Don't plot clusters with 0 et
    filt0 = cluster_pt > 0
    cluster_pt = cluster_pt[filt0]
    cluster_eta = cluster_eta[filt0]
    cluster_phi = cluster_phi[filt0]

    #Plot clusters
    fig,ax = plt.subplots(1,1,figsize=(7,4.7))
    cluster_scatter = ax.scatter(ak.ravel(cluster_eta), ak.ravel(cluster_phi), c=ak.ravel(cluster_pt), marker='.', vmin=0.0, vmax=max_et)
    ax.set_xlim(-ECAL_ETA_RANGE,ECAL_ETA_RANGE)
    ax.set_ylim(-np.pi,np.pi)
    plt.colorbar(cluster_scatter)
    if is_new:
        ax.set_title(f"New Clusters (Event {iEvent})")
    else:
        ax.set_title(f"Old Clusters (Event {iEvent})")
    ax.set_xlabel(r"$\eta$",fontsize=10)
    ax.set_ylabel(r"$\phi$",fontsize=10,labelpad=2)

    #Add lines showing where RCT cards are
    ax.plot((0,0),(-np.pi,np.pi),color='black')
    if is_new:
        half_n_cards = 12
    else:
        half_n_cards = 18
    for i in range(half_n_cards - 1):
        phi_line = -np.pi + (2*np.pi*(i+1)/half_n_cards)
        ax.plot((-ECAL_ETA_RANGE,ECAL_ETA_RANGE),(phi_line,phi_line),color='black')

    if is_new:
        outfile = f"{PLOT_DIR}new_clusters.png"
    else:
        outfile = f"{PLOT_DIR}old_clusters.png"
    plt.savefig(outfile,bbox_inches='tight')
    print(f"Saved output to {outfile}")

def plot_towers(tree,is_new,iEvent=0,max_et=None):
    if is_new:
        et = tree["tower_et"].array()*0.5 #Account for LSB
    else:
        ecal_et = tree["tower_ecal_et"].array()
        hcal_et = tree["tower_hcal_et"].array()
        et = ecal_et + hcal_et
    iEta = tree["tower_eta"].array()
    iPhi = tree["tower_phi"].array()

    if is_new:
        #Convert new emulator coordinates from relative to card to global
        iEta = convert_tower_global_iEta(iEta)
        iPhi = convert_tower_global_iPhi(iPhi)

    et = et[iEvent]
    iEta = iEta[iEvent]
    iPhi = iPhi[iEvent]

    eta = -ECAL_ETA_RANGE + TOWER_SIZE_ETA*(iEta + 0.5)
    phi = TOWER_SIZE_PHI*(iPhi + 0.5) - np.pi
    #Shift values > 2*pi to be < 0
    phi = ak.where(phi < np.pi, phi, phi - 2*np.pi)

    eta_axis = axis.Regular(TOWERS_ETA_PER_CARD*2,-ECAL_ETA_RANGE,ECAL_ETA_RANGE,name="eta",label=r"$\eta$")
    phi_axis = axis.Regular(72,-np.pi,np.pi,name="phi",label=r"$\phi$")
    display_hist = Hist(eta_axis,phi_axis,name="Events")

    display_hist.fill(eta=ak.ravel(eta),phi=ak.ravel(phi),weight=ak.ravel(et))


    fig,ax = plt.subplots(1,1,figsize=(7,4.7))
    hep.hist2dplot(display_hist,ax=ax,cmin=0.0,cmax=max_et)
    if is_new:
        title = f"New towers (Event {iEvent})"
    else:
        title = f"Old towers (Event {iEvent})"
    ax.set_title(title,y=1.07,pad=2)
    ax.set_xlabel(r"$\eta$",fontsize=10)
    ax.set_ylabel(r"$\phi$",fontsize=10,labelpad=2)

    #Add lines showing where RCT cards are
    ax.plot((0,0),(-np.pi,np.pi),color='black')
    if is_new:
        half_n_cards = 12
    else:
        half_n_cards = 18
    for i in range(half_n_cards - 1):
        phi_line = -np.pi + (2*np.pi*(i+1)/half_n_cards)
        ax.plot((-ECAL_ETA_RANGE,ECAL_ETA_RANGE),(phi_line,phi_line),color='black')

    if is_new:
        outfile = f"{PLOT_DIR}new_towers.png"
    else:
        outfile = f"{PLOT_DIR}old_towers.png"
    plt.savefig(outfile,bbox_inches='tight')
    print(f"Saved output to {outfile}")

def plot_both(tree,is_new,iEvent=0,max_et=None):
    #Do towers
    if is_new:
        et = tree["tower_et"].array()*0.5 #Account for LSB
    else:
        ecal_et = tree["tower_ecal_et"].array()
        hcal_et = tree["tower_hcal_et"].array()
        et = ecal_et + hcal_et
    iEta = tree["tower_eta"].array()
    iPhi = tree["tower_phi"].array()

    if is_new:
        #Convert new emulator coordinates from relative to card to global
        iEta = convert_tower_global_iEta(iEta)
        iPhi = convert_tower_global_iPhi(iPhi)

    et = et[iEvent]
    iEta = iEta[iEvent]
    iPhi = iPhi[iEvent]

    eta = -ECAL_ETA_RANGE + TOWER_SIZE_ETA*(iEta + 0.5)
    phi = TOWER_SIZE_PHI*(iPhi + 0.5) - np.pi
    #Shift values > 2*pi to be < 0
    phi = ak.where(phi < np.pi, phi, phi - 2*np.pi)

    eta_axis = axis.Regular(TOWERS_ETA_PER_CARD*2,-ECAL_ETA_RANGE,ECAL_ETA_RANGE,name="eta",label=r"$\eta$")
    phi_axis = axis.Regular(72,-np.pi,np.pi,name="phi",label=r"$\phi$")
    display_hist = Hist(eta_axis,phi_axis,name="Events")

    display_hist.fill(eta=ak.ravel(eta),phi=ak.ravel(phi),weight=ak.ravel(et))


    fig,ax = plt.subplots(1,1,figsize=(7,4.7))
    hep.hist2dplot(display_hist,ax=ax,cmin=0.0,cmax=max_et)

    #Do clusters

    cluster_pt = tree["cluster_pt"].array()
    if is_new:
        cluster_pt = cluster_pt*0.5 #account for LSB
    cluster_eta = tree["cluster_eta"].array()
    cluster_phi = tree["cluster_phi"].array()

    if is_new:
        cluster_eta = convert_to_eta(cluster_eta)
        cluster_phi = convert_to_phi(cluster_phi)

    cluster_pt = cluster_pt[iEvent]
    cluster_eta = cluster_eta[iEvent]
    cluster_phi = cluster_phi[iEvent]

    #Don't plot clusters with 0 et
    filt0 = cluster_pt > 0
    cluster_pt = cluster_pt[filt0]
    cluster_eta = cluster_eta[filt0]
    cluster_phi = cluster_phi[filt0]

    #Plot clusters
    cluster_scatter = ax.scatter(ak.ravel(cluster_eta), ak.ravel(cluster_phi), c='red', marker='.')

    #Do some labeling

    ax.set_xlim(-ECAL_ETA_RANGE,ECAL_ETA_RANGE)
    ax.set_ylim(-np.pi,np.pi)

    if is_new:
        title = f"New emulator (Event {iEvent})"
    else:
        title = f"Old emulator (Event {iEvent})"
    ax.set_title(title,y=1.07,pad=2)
    ax.set_xlabel(r"$\eta$",fontsize=10)
    ax.set_ylabel(r"$\phi$",fontsize=10,labelpad=2)

    #Add lines showing where RCT cards are
    ax.plot((0,0),(-np.pi,np.pi),color='black')
    if is_new:
        half_n_cards = 12
    else:
        half_n_cards = 18
    for i in range(half_n_cards - 1):
        phi_line = -np.pi + (2*np.pi*(i+1)/half_n_cards)
        ax.plot((-ECAL_ETA_RANGE,ECAL_ETA_RANGE),(phi_line,phi_line),color='black')

    if is_new:
        outfile = f"{PLOT_DIR}new_emulator.png"
    else:
        outfile = f"{PLOT_DIR}old_emulator.png"
    plt.savefig(outfile,bbox_inches='tight')
    print(f"Saved output to {outfile}")

######## Plotting and testing

def plot(h,outfile,title=None,xlabel=None,logy=False,flow="hint"):
    fig, ax = plt.subplots(1,1,figsize=(7,4.7))
    
    hep.histplot(h,ax=ax,flow=flow,yerr=False)

    if title is not None:
        ax.set_title(title,pad=2)
    ax.set_xlabel(xlabel,fontsize=10,labelpad=2)
    if logy:
        ax.semilogy()
    plt.savefig(outfile)
    print(f"Saved output to {outfile}")
    plt.close()

def test_convert_to_eta():
    test_in = ak.Array([[[1,2,3],[4,5,16]]])
    print(test_in.show())
    test_out = convert_to_eta(test_in,n_cards=2)
    print("-----------------------")
    print(test_out.show())

def test_convert_to_phi():
    test_in = ak.Array([[[1,2,3],[4,5,16]]])
    print(test_in.show())
    test_out = convert_to_phi(test_in,n_cards=2)
    print("-----------------------")
    print(test_out.show())

############## Main

def main():
    new_tree = get_new_tree()
    old_tree = get_old_tree()

    pt_diff, eta_diff, phi_diff = compare_clusters(old_tree,new_tree)
    et_rel_diff, et_diff, hoe_diff = compare_towers(old_tree,new_tree)

    filt = phi_diff > -1

    pt_rel_diff_axis = axis.Regular(20,-1,1,name="pt_rel_diff",label=r"$p_t$ Relative Difference [GeV]")
    pt_diff_axis = axis.Regular(21,-5.25,5.25,name="pt_diff",label=r"$p_t$ Difference [GeV]")
    phi_diff_axis = axis.Regular(64,0,3.2,name="phi_diff",label=r"$\phi$ Difference")
    eta_diff_axis = axis.Regular(120,-3,3,name="eta_diff",label=r"$\eta$ Difference")
    hoe_diff_axis = axis.Regular(44,-1.0,1.2,name="hoe_diff",label="HOE Difference")

    pt_axis = axis.Regular(20,0,100,name="pt",label="pt")
    et_axis = axis.Regular(40,0,20,name="et",label="et")
    phi_axis = axis.Regular(32,-3.2,3.2,name="phi",label="phi")
    eta_axis = axis.Regular(15,-1.5,1.5,name="eta",label="eta")
    hoe_axis = axis.Regular(24,0.0,1.2,name="hoe",label="hoe")

    #Cluster difference histograms
    pt_hist = Hist(pt_diff_axis,name="Events")
    eta_hist = Hist(eta_diff_axis,name="Events")
    phi_hist = Hist(phi_diff_axis,name="Events")

    #Cluster histograms
    new_pt_hist = Hist(pt_axis,name="Events")
    old_pt_hist = Hist(pt_axis,name="Events")
    new_phi_hist = Hist(phi_axis,name="Events")
    old_phi_hist = Hist(phi_axis,name="Events")
    new_eta_hist = Hist(eta_axis,name="Events")
    old_eta_hist = Hist(eta_axis,name="Events")

    #Tower difference histograms
    tower_et_rel_hist = Hist(pt_rel_diff_axis,name="Events")
    tower_et_hist = Hist(pt_diff_axis,name="Events")
    tower_hoe_hist = Hist(hoe_diff_axis,name="Events")

    #Tower histograms
    new_tower_et_hist = Hist(et_axis,name="Events")
    old_tower_et_hist = Hist(et_axis,name="Events")
    new_tower_hoe_hist = Hist(hoe_axis,name="Events")
    old_tower_hoe_hist = Hist(hoe_axis,name="Events")

    pt_hist.fill(pt_diff=ak.ravel(pt_diff[filt]))
    eta_hist.fill(eta_diff=ak.ravel(eta_diff[filt]))
    phi_hist.fill(phi_diff=ak.ravel(phi_diff[filt]))

    tower_et_rel_hist.fill(pt_rel_diff=ak.ravel(et_rel_diff))
    tower_et_hist.fill(pt_diff=ak.ravel(et_diff))
    tower_hoe_hist.fill(hoe_diff=ak.ravel(hoe_diff))

    new_pt, new_eta, new_phi = get_new_top2_clusters(new_tree)
    old_pt, old_eta, old_phi = get_old_top2_clusters(old_tree)

    new_pt_hist.fill(ak.ravel(new_pt[filt]))
    new_phi_hist.fill(ak.ravel(new_phi[filt]))
    new_eta_hist.fill(ak.ravel(new_eta[filt]))
    old_pt_hist.fill(ak.ravel(old_pt[filt]))
    old_phi_hist.fill(ak.ravel(old_phi[filt]))
    old_eta_hist.fill(ak.ravel(old_eta[filt]))

    new_et, new_hoe = get_new_et_hoe(new_tree)
    old_et, old_hoe = get_old_et_hoe(old_tree)

    new_tower_et_hist.fill(new_et)
    new_tower_hoe_hist.fill(new_hoe)
    old_tower_et_hist.fill(old_et)
    old_tower_hoe_hist.fill(old_hoe)

    Path("plots").mkdir(parents=True,exist_ok=True)

    iEvent = 0
    new_cluster_et = new_tree["cluster_pt"].array()[iEvent]*0.5
    new_cluster_et = new_cluster_et[new_cluster_et > 0] #drop 0 GeV clusters
    old_cluster_et = old_tree["cluster_pt"].array()[iEvent]
    new_max_et = ak.max(new_cluster_et)
    old_max_et = ak.max(old_cluster_et)
    if new_max_et is None:
        if old_max_et is None:
            max_cluster_et = None
        else:
            max_cluster_et = old_max_et
    else:
        if old_max_et is None:
            max_cluster_et = new_max_et
        else:
            max_cluster_et = max(new_max_et,old_max_et)
    plot_clusters(new_tree,True,iEvent=iEvent,max_et=max_cluster_et)
    plot_clusters(old_tree,False,iEvent=iEvent,max_et=max_cluster_et)

    new_tower_et = new_tree["tower_et"].array()[iEvent]*0.5
    old_tower_et = old_tree["tower_ecal_et"].array()[iEvent] + old_tree["tower_hcal_et"].array()[iEvent]
    max_tower_et = max(ak.max(new_tower_et),ak.max(old_tower_et))
    plot_towers(new_tree,True,iEvent=iEvent,max_et=max_tower_et)
    plot_towers(old_tree,False,iEvent=iEvent,max_et=max_tower_et)

    plot_both(new_tree,True,iEvent=iEvent,max_et=max_tower_et)
    plot_both(old_tree,False,iEvent=iEvent,max_et=max_tower_et)

    plot(pt_hist,f"{PLOT_DIR}pt_diff.png",r"Cluster $p_t$ Relative Difference",xlabel=r"$p_t$ Rel. Diff. [GeV]")
    plot(eta_hist,f"{PLOT_DIR}eta_diff.png",r"Cluster $\eta$ Difference",xlabel=r"$\eta$ Diff.")
    plot(phi_hist,f"{PLOT_DIR}phi_diff.png",r"Cluster$\phi$ Difference",xlabel=r"$\phi$ Diff.")

    plot(new_pt_hist,f"{PLOT_DIR}new_pt.png",r"New $p_t$",xlabel=r"$p_t$ [GeV]")
    plot(old_pt_hist,f"{PLOT_DIR}old_pt.png",r"Old $p_t$",xlabel=r"$p_t$ [GeV]")
    plot(new_eta_hist,f"{PLOT_DIR}new_eta.png",r"New $\eta$",xlabel=r"$\eta$")
    plot(old_eta_hist,f"{PLOT_DIR}old_eta.png",r"Old $\eta$",xlabel=r"$\eta$")
    plot(new_phi_hist,f"{PLOT_DIR}new_phi.png",r"New $\phi$",xlabel=r"$\phi$")
    plot(old_phi_hist,f"{PLOT_DIR}old_phi.png",r"Old $\phi$",xlabel=r"$\phi$")

    plot(tower_et_rel_hist,f"{PLOT_DIR}tower_et_rel_diff.png",rf"Tower $E_T$ Relative Difference (E >= {TOWER_DIFF_MIN_ENERGY} GeV)",xlabel=r"$E_T$ Rel. Diff. [GeV]")
    plot(tower_et_hist,f"{PLOT_DIR}tower_et_diff.png",rf"Tower $E_T$ Difference (E >= {TOWER_DIFF_MIN_ENERGY} GeV)",xlabel=r"$E_T$ Diff. [GeV]")
    plot(tower_hoe_hist,f"{PLOT_DIR}tower_hoe_diff.png",rf"Tower HOE Difference (E >= {TOWER_DIFF_MIN_ENERGY} GeV)",xlabel="HOE Diff.")
    
    plot(new_tower_et_hist,f"{PLOT_DIR}new_tower_et.png",r"New $E_T$",xlabel=r"$E_T$ [GeV]")
    plot(old_tower_et_hist,f"{PLOT_DIR}old_tower_et.png",r"Old $E_T$",xlabel=r"$E_T$ [GeV]")
    plot(new_tower_hoe_hist,f"{PLOT_DIR}new_tower_hoe.png","New HOE",xlabel="New HOE")
    plot(old_tower_hoe_hist,f"{PLOT_DIR}old_tower_hoe.png","Old HOE","Old HOE")

if __name__ == "__main__":
    main()
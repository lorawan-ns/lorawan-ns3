import numpy as np
import matplotlib as mpl
#mpl.use("agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
#from tkinter import Tk
#from tkinter.filedialog import askopenfilename
import scipy.interpolate as interp
import sys
plt.style.use('classic')


filename = 'lora_2000'

def parser():
    lines = []
    with open(filename) as f:
        lines = [i.split() for i in f.readlines() if 'Waf' not in i and 'build' not in i]
    nDevices = [int(i[0]) for i in lines]
    prob_rx = [float(i[2])*100 for i in lines]
    prob_int = [float(i[3])*100 for i in lines]
    prob_norx = [float(i[4])*100 for i in lines]
    plt.plot(nDevices,prob_rx, color='g', label='received/nDevices')
    plt.plot(nDevices,prob_int,'k--', color='blue', label='interfered/nDevices')
    plt.plot(nDevices,prob_norx,'k:', color='orange',label='noMoreReceivers/nDevices')
    plt.xlabel('Number of Devices')
    plt.ylabel('Prob. Packets Received %')
    legend = plt.legend(loc='center left', shadow=True, fontsize='small')
    plt.grid(True)
    plt.savefig('teste_2k.png')

    plt.figure()
    x = []
    for i in 1000*np.arange(1,11):
        rec = [k[1] for k in zip(nDevices, prob_rx) if k[0] == i]
        x.append(rec)
    #plt.errorbar(1000*np.arange(1,11), [np.mean(i) for i in x], yerr=[np.std(i) for i in x], fmt="rs--", linewidth=3)
    plt.boxplot(x, 1)
    plt.xticks(range(1,11), 1000*np.arange(1,11))
    plt.plot(np.arange(1,11), [np.mean(i) for i in x], "--", alpha=0.5)
    plt.ylim(0,100)
    plt.xlabel("Number of Devices")
    plt.ylabel("Received")
    plt.grid(True)
    plt.show()
parser()


#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys, math
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import re
from matplotlib.backends.backend_pdf import PdfPages

make_pdfs = True
label_time = False
pdfdir = "pdfs"

def cdf(data):

    data_size=len(data)

    # Set bins edges
    data_set=sorted(data)
    bins=np.append(data_set, data_set[-1]+1)

    # Use the histogram function to bin the data
    counts, bin_edges = np.histogram(data, bins=bins, density=False)

    counts=counts.astype(float)/data_size

    # Find the cdf
    cdf = np.cumsum(counts)

    #return x,y values
    return bin_edges[0:-1],cdf

def get_data(line):
    d = []
    if line.startswith("#"):
        return None
    if line.find("Ticks") > -1:
        parts = line.split(":")[1].split(",")
        for p in parts:
            d.append(int(p))
    if len(d) == 0:
        return None
    return d

def get_legend(first_line):
    if not first_line.startswith("#"):
        return []
    parts = first_line[1:-1].split(",")
    return parts

def main(file_names):
    if len(file_names) == 0:
        sys.exit()
    else:
        for file_name in file_names:
            plot_data(file_name)

def sample_2us(v):
    if v == 0:
        return 0
    return (1.5*v)/1000

def plot_data(file_name):
    global label_time
    data = []
    num_data_fields = 0
    legend = None
    with open(file_name) as f:
        for line in f:
            if legend is None:
                legend = get_legend(line)
            d = get_data(line)
            if d is None:
                continue
            data.append(d)

    lists = zip(*data)
    sname = file_name.split("/")[-1]
    if len(legend) != len(lists):
        legend = ["?"] * len(lists)

    for l,t in zip(lists,legend):
        x,y = cdf(l)
        if label_time is True:
            x = [sample_2us(v) for v in x]
            
        plot(x,y, sname + "-" + t)

def plot(x,y, title):
    global make_pdfs
    global label_time
    plt.plot(x,y,linestyle='--', marker="+", color='b')
    plt.ylabel("CDF")
    if label_time is True:
        plt.ticklabel_format(style='plain',useOffset=False)
        plt.xlabel("usec")
    else:
        plt.xlabel("Cycles")
    plt.title(title)
    plt.grid(True)

    if not make_pdfs:
        plt.show()
        plt.close()
    else:
        with PdfPages( pdfdir + "/" + title + ".pdf") as pdf:
            pdf.savefig()
            plt.close()
    


if __name__ == "__main__":
    if len(sys.argv) == 0:
        sys.exit()

    main(sys.argv[1:])

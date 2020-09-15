#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys, math
import numpy as np

def main(file_names):
    if len(file_names) == 0:
        sys.exit()
    else:
        for file_name in file_names:
            process_data(file_name)


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

def process_data(file_name):
    num_data_fields = 0
    data = []
    legend = None

    with open(file_name) as f:
        for line in f:
            if legend is None:
                legend = get_legend(line)
            d = get_data(line)
            if d is None:
                continue
            data.append(d)
            num_fields = len(d)
            if num_fields > num_data_fields:
                num_data_fields = num_fields

    data = zip(*data)

    averages = calculate_avgs(data)
    standard_deviations = calculate_std_deviations(data)
    maxes = calculate_max(data)
    mins = calculate_min(data)
    p95s = calculate_percentile(data,95)

    if len(legend) != len(averages):
        legend = ["?"] * len(averages)

    print "File: " + str(file_name)
    for avg,std,legend,mx,mi,p,d in zip(averages,standard_deviations,legend,maxes,mins,p95s,data):
        print "\t" + legend
	print "\t\tAverage: " + str(avg) + "+-" + str(std) + " cycles,  (" + str(cycles_to_us(avg)) + "+-" + str(cycles_to_us(std)) + " us)"
        print "\t\tMax: " + str(mx) + "\tMin: " + str(mi)
        print "\t\t95th Percentile: " + str(p)
        print "\t\tSamples: " + str(len(d))

def calculate_avgs(iterations):
    averages = []

    for iteration in iterations:
        averages.append(sum(iteration)/len(iteration))
    return averages

def calculate_std_deviations(iterations):
    stdevs = []
    for iteration in iterations:
        stdevs.append(np.std(iteration))
    return stdevs

def calculate_max(iterations):
    maxes = []
    for it in iterations:
        maxes.append(max(it))
    return maxes
        
def calculate_min(iterations):
    mins = []
    for it in iterations:
        mins.append(min(it))
    return mins

def calculate_percentile(iterations, p):
    ps = []
    for it in iterations:
        ps.append(np.percentile(it, p))
    return ps

def cycles_to_us(cycles):
    ns = cycles*1.5
    return float(ns)/(1000)

if __name__ == "__main__":
    if len(sys.argv) == 0:
        sys.exit()

    main(sys.argv[1:])

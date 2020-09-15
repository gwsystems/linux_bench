#!/usr/bin/python
# Parse results from a new benchmark run into separate files for each benchmark
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import os
import time
import re

def main(file_name):
    if len(file_name) != 1:
        sys.exit()
    fname = file_name[0]

    #Create timestamped directory
    time_str = time.strftime("%Y-%m-%d-%H-%M-%S")
    directory="output/{tm}".format(tm=time_str)
    if not os.path.exists("output"):
        try:
            os.mkdir("output")
        except Exception as e:
            print "Failed to create output directory"
            sys.exit()
    if not os.path.exists(directory):
        try:
            os.mkdir(directory)
        except Exception as e:
            print str(e)
            sys.exit()

    in_chunk=False
    name = None
    new_file = None
    with open(fname) as f:
        for l in f:
            if in_chunk:
                if l.strip().startswith("#############"):
                    m = re.search("###+EndBench([0-9]+.*)",l.strip())
                    if m is not None:
                        new_file.close()
                        in_chunk = False
                        new_file = None
                        name = None
                        continue
                    else:
                        print "Warning: Possible End Line!"

            if in_chunk:
                new_file.write(l.strip() +"\n")
            
            if not in_chunk:
                if l.strip().startswith("#############"):
                    m = re.search("###+BeginBench([0-9]+.*)",l.strip())
                    if m is not None:
                        name = m.group(1)
                        in_chunk = True
                        try:
                            new_file = open(directory + "/" + name, "wx")
                        except Exception as e:
                            print "Failed to create output file! %s" % (name)
                            sys.exit()
                    else:
                        print "Warning: Possible Begin Line!" + str(l.strip())


if __name__ == "__main__":
    if len(sys.argv) == 0:
        sys.exit()
    main(sys.argv[1:])

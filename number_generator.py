#!/usr/bin/python

import sys
import getopt
import random

MAX_UINT = 2 ** 64 - 1
DEFAULT_COUNT = 1000


def main(argv):
    outfilename = ''
    known_bounds = False
    lower_bound = 0
    upper_bound = MAX_UINT
    try:
        opts, args = getopt.getopt(argv, "hl:o:", ["help", "limits=", "ofile="])
    except getopt.GetoptError:
        print('./number_generator.py -o <outputfile> [-l <a,b>]')
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-h', "--help"):
            print('./number_generator.py -o <outputfile> [-l <a,b>]')
            sys.exit()
        elif opt in ("-l", "--limits"):
            known_bounds = True
            try:
                lower_bound, upper_bound = (int(i) for i in arg.split(","))
                if lower_bound < 0:
                    lower_bound = 0
                if upper_bound > MAX_UINT:
                    upper_bound = MAX_UINT
                if lower_bound > upper_bound:
                    lower_bound, upper_bound = upper_bound, lower_bound
            except ValueError:
                lower_bound, upper_bound = 0, MAX_UINT
        elif opt in ("-o", "--ofile"):
            outfilename = arg
    count = int(args[0]) if args and args[0] else DEFAULT_COUNT
    out_stream = open(outfilename, "w") if outfilename else sys.stdout
    print(count, lower_bound, upper_bound, file=out_stream)
    for i in range(count):
        print(random.randint(lower_bound, upper_bound), end=" ", file=out_stream)
    print(file=out_stream)
    out_stream.close()


if __name__ == "__main__":
    main(sys.argv[1:])

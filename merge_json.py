#!/usr/bin/env python

import argparse
import json
import sys

def print_json(data):
    print(json.dumps(data,
                     separators=(',',': '),
                     indent=4,
                     sort_keys=True))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Update a JSON file using another JSON file.')
    parser.add_argument('original_file', type=str, help='Original JSON file.')
    parser.add_argument('new_file', type=str, help='New JSON file.')
    args = parser.parse_args()

    data_orig = json.load(open(args.original_file, 'r'))
    try:
        fnew = open(args.new_file,'r')
    except IOError:
        # stdout the original
        print_json(data_orig)
        sys.exit(0)
    try:
        data_new = json.load(fnew)
    except Exception as e:
        sys.stderr.write(str(e))
        sys.stderr.write('\nERROR: %s is not valid JSON.\n' % args.new_file)
        sys.exit(1)

    # You *do not* want to do this:
    # data_orig.update(data_new)
    # You will lose more settings than you want.
    for k in data_new:
        if type(data_new[k]) == dict:
            for i in data_new[k]:
                if k not in data_orig:
                    data_orig[k] = {}
                data_orig[k][i] = data_new[k][i]
        else:
            data_orig[k] = data_new[k]

    # stdout the merged configuration
    print_json(data_orig)
    
    # write file with new data
    #with open(args.original_file, 'w') as f:
    #    f.write(json.dumps(data_orig, open(args.original_file, 'w'), separators=(',',': '), indent=4, sort_keys=True))
    #    f.write("\n")
    #    print("Updated %s with values from %s" % (args.original_file, args.new_file))

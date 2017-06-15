#!/usr/bin/env python
#
# You can use command-line tools like 'jq' but they may not
# be common or available for your system. Python is common
# though, so here is one solution.

import argparse
import json
import sys


def prettyj(data):
    return json.dumps(data, separators=(',', ': '), sort_keys=True, indent=4)


def set(d, superkey, value):
    '''
    Given a dictionary, d, that contains dictionaries, drill down
    and set the new value.
      d = dictionary
      superkey = "k1.k2.k3.k4"
      value = the value you want to set
    '''
    current = d
    keys = superkey.split('.')
    try:
        for k in keys[0:-1]:
            current = current[k]
    except KeyError:
        print("ERROR: '%s' not found among keys: %s" % (k, sorted(current.keys())))
        sys.exit(1)
    current[keys[-1]] = value
    return d


if __name__ == '__main__':
    parser = argparse.ArgumentParser(epilog='''example use: python json_set.py mbed_app.json config.version.value '"2.0"' ''')
    parser.add_argument("filename", help="Filename of json file")
    parser.add_argument("key", help="The key to change. Use period '.' character for keys within keys.")
    parser.add_argument("value", help="The value to set for the given key.")
    args = parser.parse_args()

    with open(args.filename, 'r') as f:
        data = json.load(f)
    
    # Set the value
    result = set(data, args.key, args.value)

    with open(args.filename, 'w') as f:
         f.write(prettyj(data))
         print("Updated %s with '%s' = '%s'" % (args.filename, args.key, args.value))

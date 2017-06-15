#!/usr/bin/env python

import sys

standard_f = sys.argv[1]
actual_f = sys.argv[2]

with open(standard_f, 'r') as f:
    standard_messages = [x.rstrip() for x in f.readlines()]

with open(actual_f, 'r') as f:
    actual_messages = f.read()

not_found_msgs = []
for m in standard_messages:
    #Possible improvement: use 're' and do regular expression searches
    if m not in actual_messages:
        not_found_msgs.append(m)

if not_found_msgs:
    print("ERROR: Some expected output was not seen in your %s:" % actual_f)
    print("\n".join(not_found_msgs))
    print("\nIf that missing message is ok, remove it from %s." % standard_f)
    sys.exit(1)
else:
    sys.exit(0)

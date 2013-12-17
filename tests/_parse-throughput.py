#!/bin/env python2

import fileinput, sys
import xml.etree.ElementTree as ET

mystr=""
# file to string
for line in fileinput.input():
	mystr+=line
	

root = ET.fromstring(mystr)
for test in root:
  i= test.attrib["threads"]
  for line in test.text.split("\n"):
    words=line.split()
    if len(words) == 0:
      continue
    if words[3] == "100.00":
      print i,"\t",words[1] # print throughput
      #sys.stdout.write(words[1]) # print throughput
      #sys.stdout.write("\n") # print throughput
      break
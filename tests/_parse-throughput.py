#!/bin/env python2

import fileinput, sys
import xml.etree.ElementTree as ET

mystr=""

for line in fileinput.input():
	mystr+=line
	

#print mystr

root = ET.fromstring(mystr)

for test in root:
  for line in test.text.split("\n"):
    words=line.split()
    if len(words) == 0:
      continue
    if words[3] == "100.00":
      sys.stdout.write(words[1]) # print throughput
      sys.stdout.write("\n") # print throughput
      #print words[1]
      break
#!/bin/bash

ls -1 *.yaml | sed "s/.yaml//g" > devices.csv

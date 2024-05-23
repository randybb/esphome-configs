#!/bin/bash

export ESPHOME_IS_HA_ADDON=true

while IFS=, read -r device
do
    esphome upload ${device}.yaml --device ${device}.local
#done < devices_nspanel.csv
done < devices_prod.csv

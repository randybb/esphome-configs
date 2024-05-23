#!/bin/bash

export ESPHOME_IS_HA_ADDON=true

while IFS=, read -r device
do
    esphome compile ${device}.yaml
#    esphome upload ${device}.yaml --device ${device}.local
#    esphome run ${device}.yaml
done < devices_dev.csv
# done < devices_prod.csv
#done < devices_nspanel.csv

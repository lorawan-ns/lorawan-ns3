#!/bin/bash
#===============================================================================
#
#          FILE:  lora.sh
# 
#         USAGE:  ./lora.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR:  (Nagib Matni), 
#       COMPANY:  
#       VERSION:  1.0
#       CREATED:  05/09/2018 10:48:48 CST
#      REVISION:  ---
#===============================================================================
#awk '/seed/ || /CalculateDataRateIndexPER/ {print}' verbose.teste | sed -e 's/SF=/SF = /g;s/pos=/pos = /g;s/d=/d = /g;s/(//g;s/)//g;s/,/ /g;s/PER=/PER = /g;s/%/ % /g;s/\([0-9]\)\([a-z]\)/\1 \2/g' > verbose_2
gatewayRings=1
radius=2000
simulationTime=600.0
randomSeed=12345
nRuns=30

for i in {9000..10000..1000}
do
nDevices=$i
./waf --run="complete-lorawan-network-example --nDevices=$nDevices \
									--gatewayRings=$gatewayRings \
									--simulationTime=$simulationTime \
									--randomSeed=$randomSeed \
									--nRuns=$nRuns \
									--radius=$radius " >> "lora_$radius"
done


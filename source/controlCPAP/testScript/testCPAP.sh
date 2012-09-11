#!/bin/sh

while true
do
    gettimeofday send
    echo `curl http://192.168.1.103/controlCPAP?cmd=CPAP_GET_PRESS -s`
done

#!/bin/bash

IP=$1

if [ "$IP" = "" ]; then 
    echo "Usage: sudo ./run <ip_machine>"
    echo "e.g: sudo ./run 192.168.0.1"
    exit 1
fi

sudo docker run -itd --log-driver syslog --log-opt syslog-address=tcp://$IP:514 --name pipes pipe
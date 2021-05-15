#!/bin/bash

FILENAME="daemon.json"
DOCKER_DIR="/etc/docker"
IP=$1

if [ "$IP" = "" ]; then 
    echo "Usage: sudo ./config <ip_hostmachine>"
    echo "e.g: sudo ./config 192.168.0.1"
    echo "     sudo ./config 127.0.0.1"
    exit 1
fi

if [ -f $FILENAME ]; then
    rm $FILENAME
fi

touch $FILENAME

echo '{' >> $FILENAME
echo '  "log-driver": "syslog",' >> $FILENAME
echo '  "log-opts": {' >> $FILENAME
echo '    "syslog-address": "tcp://'$IP':514",' >> $FILENAME
echo '    "tag": "container_name/{{.Name}}",' >> $FILENAME
echo '    "labels": "dev",' >> $FILENAME
echo '    "syslog-facility": "daemon"' >> $FILENAME
echo '  }' >> $FILENAME
echo '}' >> $FILENAME
echo ' ' >> $FILENAME

if [ ! -d $DOCKER_DIR ]; then 
    sudo mkdir -p $DOCKER_DIR
fi

sudo cp $FILENAME $DOCKER_DIR

echo "Enabling Syslog through TCP"

sed -i '/module(load="imtcp")/s/^#//g' /etc/rsyslog.conf
sed -i '/input(type="imtcp" port="514")/s/^#//g' /etc/rsyslog.conf

echo "----> Restarting services"
sudo systemctl restart docker 
sudo systemctl restart rsyslog

echo "<##########-------------------########## Done! ##########-------------------##########>"
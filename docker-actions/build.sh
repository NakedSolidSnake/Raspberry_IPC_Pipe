#!/bin/bash

echo "Generating image ..."
cd ..
sudo docker build -t pipes .
sudo docker images | grep pipes
echo "<##########-------------------########## Done! ##########-------------------##########>"

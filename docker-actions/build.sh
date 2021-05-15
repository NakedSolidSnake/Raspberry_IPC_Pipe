#!/bin/bash

echo "Generating image ..."
cd ..
sudo docker build -t pipe .
sudo docker images | grep pipes
echo "<##########-------------------########## Done! ##########-------------------##########>"

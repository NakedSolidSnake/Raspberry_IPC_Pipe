#!/bin/bash

touch file 
echo "0" > file

# Copy data to docker using tee
sudo docker exec -i pipes tee /tmp/pipe_file < file > /dev/null
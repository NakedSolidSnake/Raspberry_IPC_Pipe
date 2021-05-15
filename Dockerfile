FROM debian

MAINTAINER Cristiano Silva de Souza, cristianosstec@gmail.com

ENV PATH=$PATH:/pipe/build/bin

RUN apt-get update -y
RUN apt-get update -y
RUN apt-get install build-essential -y
RUN apt-get install cmake -y


# install pipe
COPY . /pipe
RUN cd pipe && mkdir build && cd build && cmake -DARCH=PC  -DOCKER_MODE=ON .. && make
WORKDIR /pipe/build/bin

ENTRYPOINT ["bash", "-c", "launch_processes"]

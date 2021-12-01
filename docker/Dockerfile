FROM ubuntu:latest

ADD sources.list /etc/apt/

RUN apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt install -y tzdata

# General dependencies
RUN apt-get install -y \
  git \
  mercurial 
RUN apt-get install -y \
  wget \
  vim \
  autoconf \
  bzr \
  cvs 
RUN apt-get install -y \
  unrar \
  build-essential \
  clang \
  valgrind \
  gsl-bin 
RUN apt-get install -y \
  libgslcblas0 \
  libgsl-dev \
  flex \
  bison \
  libfl-dev \
  tcpdump \
  sqlite 
RUN apt-get install -y \
  sqlite3 \
  libsqlite3-dev \
  libxml2 \
  libxml2-dev \
  vtun \
  lxc

# QT4 components
RUN apt-get install -y \
  qtbase5-dev

# Python components
RUN apt-get install -y \
  python \
  python-dev \
  python-setuptools \
  cmake \
  libc6-dev \
  libc6-dev-i386 \
  g++-multilib

# NS-3

# Create working directory
RUN mkdir -p /usr/ns3
WORKDIR /usr

# Fetch NS-3 source
RUN wget http://www.nsnam.org/release/ns-allinone-3.30.tar.bz2
RUN tar -xf ns-allinone-3.30.tar.bz2

# Configure and compile NS-3
RUN cd ns-allinone-3.30&& ./build.py --enable-examples --enable-tests

RUN ln -s /usr/ns-allinone-3.30/ns-3.30/ /usr/ns3/

# Cleanup
RUN apt-get clean && \
  rm -rf /var/lib/apt && \
  rm /usr/ns-allinone-3.30.tar.bz2

RUN apt update
RUN apt install unzip

RUN cd /usr/ns-allinone-3.30/ns-3.30/contrib && git clone https://gitee.com/lyon1998/ns3-ai
RUN apt install python3 python3-pip -y
RUN cd /usr/ns-allinone-3.30/ns-3.30 && ./waf configure && ./waf
RUN cd /usr/ns-allinone-3.30/ns-3.30/contrib/ns3-ai/py_interface && pip3 install . --user

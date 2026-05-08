FROM ubuntu:24.04

RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  iperf3 \
  iptables \
  libmnl-dev \
  libnetfilter-queue-dev \
  psmisc \
  python3 \
  python3-matplotlib \
  python3-numpy \
  sudo \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/5g-network-emulator
COPY . .

RUN make -j"$(nproc)" && make test

FROM ubuntu:24.04

# Update and install packages
RUN apt-get update && apt-get install -y \
  build-essential libnetfilter-queue-dev libmnl-dev \
  vim

# Copy source
WORKDIR /usr/src/5g-network-emulator
COPY . .

# Build emulator
RUN ls -lrt
RUN make

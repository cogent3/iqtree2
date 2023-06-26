# Use the official Debian image as the base image
FROM debian:latest

# Set environment variables to non-interactive
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    cmake \
    git \
    libboost-all-dev \
    libeigen3-dev \
    zlib1g-dev \
    libbz2-dev \
    liblzma-dev \
    libtbb-dev \
    libgoogle-perftools-dev \
    libopenmpi-dev \
    openmpi-bin \
    libpll-dev \
    wget

# Download header files for plf_nanotimer and plf_colony
RUN mkdir -p /usr/local/include/plf && \
    wget -O /usr/local/include/plf/plf_nanotimer.h https://raw.githubusercontent.com/mattreecebentley/plf_nanotimer/master/plf_nanotimer.h && \
    wget -O /usr/local/include/plf/plf_colony.h https://raw.githubusercontent.com/mattreecebentley/plf_colony/master/plf_colony.h

# Set the working directory
WORKDIR /iqtree2

# Clone LSD2 into the expected subdirectory after copying the contents
RUN git clone https://github.com/tothuhien/lsd2.git lsd2

# Copy entrypoint script to pass keys from the host (specified in SSH_KEYS_SOURCE_DIR) to the container
COPY docker/entrypoint.sh /entrypoint.sh

# Set permissions for the entrypoint script
RUN chmod +x /entrypoint.sh

# Set the entrypoint
ENTRYPOINT ["/entrypoint.sh"]

# Run an interactive shell when the container launches
CMD ["/bin/bash"]

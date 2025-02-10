FROM debian:bookworm

# Prevents debconf: unable to initialize frontend: Dialog
ENV DEBIAN_FRONTEND=noninteractive

# Update packages
RUN apt-get update && apt-get upgrade -y

# Install dependencies
RUN apt-get install -y build-essential autoconf automake bison flex gawk libtool libtool-bin libncurses-dev libgmp-dev libarchive-dev nettle-dev zlib1g-dev curl file git gperf help2man texinfo unzip wget 7zip

# Create a non-root user (crosstool-ng doesn't run as root)
RUN adduser --disabled-password --gecos '' user

# This is needed because the SDK script uses sudo
# Install sudo
RUN apt-get install -y sudo
# Add the user to the sudo group
RUN usermod -aG sudo user
# Allow the user to run sudo without a password
RUN echo "user ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Switch to the non-root user
USER user

# Switch to the user's home directory
WORKDIR /home/user

# Clone the toolchain repository
RUN git clone --recursive --depth=1 https://github.com/koreader/koxtoolchain.git

# Build the toolchain
RUN cd koxtoolchain && ./gen-tc.sh kindlepw2

# Clone the SDK repository
RUN git clone --recursive --depth=1 https://github.com/KindleModding/kindle-sdk.git

# Copy the patch file
COPY kindle-sdk-nomount.patch /home/user/kindle-sdk-nomount.patch

# Patch and build the SDK
RUN cd kindle-sdk && git apply /home/user/kindle-sdk-nomount.patch && ./gen-sdk.sh kindlepw2

# Copy the bash environment file
COPY bash-env.sh /home/user/bash-env.sh

# Source the bash environment file in the user's bashrc
RUN echo "source /home/user/bash-env.sh" >> /home/user/.bashrc

# Set the environment file as BASH_ENV
ENV BASH_ENV=/home/user/bash-env.sh

# Volume for the source code
VOLUME /source

# Set the default command
ENTRYPOINT ["/bin/bash"]

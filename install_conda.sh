#!/bin/bash
# Script to install Miniconda and create Python 3 environment

echo "Installing Miniconda..."

# Download Miniconda installer
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh

# Install Miniconda
bash ~/miniconda.sh -b -p $HOME/miniconda3

# Initialize conda
$HOME/miniconda3/bin/conda init bash

# Source bashrc to make conda available
source ~/.bashrc

# Create Python 3 environment
$HOME/miniconda3/bin/conda create -n py3 python=3.9 -y

echo "Conda installation complete!"
echo "To activate the environment, run: conda activate py3"


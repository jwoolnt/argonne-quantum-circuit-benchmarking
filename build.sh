#!/bin/bash

# Check if input file is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <input_file>"
  exit 1
fi

INPUT="$1"

# Create output directories
mkdir -p qbuild Visualization

# Run the intel-quantum-compiler command
intel-quantum-compiler -o qbuild -P json "$INPUT"

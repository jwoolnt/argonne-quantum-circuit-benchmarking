# Use Intel Quantum SDK Image
FROM intellabs/intel_quantum_sdk:latest

# Install Python Tools
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	ca-certificates \
	git \
	vim

# Install Pandas & MatPlotLib
RUN /opt/intel/quantum-sdk/latest/virtualenv/bin/pip install \
	pandas \
	matplotlib

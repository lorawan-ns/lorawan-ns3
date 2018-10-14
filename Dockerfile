FROM ubuntu:16.04
MAINTAINER Lucas Pacheco 

RUN apt update
RUN apt install git gcc g++ python-dev python -y
RUN git clone https://github.com/lorawan-ns/ns-3-dev-git lora-ns3
WORKDIR lora-ns3
RUN ./waf configure --enable-examples
RUN ./waf
CMD ["sh", "-c", "git pull && bash"]

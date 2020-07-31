FROM ubuntu:18.04

RUN apt-get update && \
    apt-get upgrade --yes && \
    apt-get install --yes wget &&\
    wget https://admin.hca.bsc.es/epi/ftp/llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    tar -xjf llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    rm llvm-EPI-0.7-release-toolchain-cross-2020-03-09-1237.tar.bz2 && \
    wget https://admin.hca.bsc.es/epi/ftp/llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2 && \
    tar -xjf llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2 && \
    rm llvm-EPI-release-toolchain-cross-2020-05-12-1155.tar.bz2

FROM debian

RUN apt-get update && apt-get install -y wget

ENV MY_INSTALL_DIR="/.local"
#RUN echo "$MY_INSTALL_DIR"
RUN mkdir -p $MY_INSTALL_DIR
ENV PATH="$PATH:$MY_INSTALL_DIR/bin"
#RUN echo "$PATH"

#RUN sudo apt install wget

RUN apt-get install -y cmake

RUN cmake --version

RUN apt-get install -y build-essential autoconf libtool pkg-config git libssl-dev

# install protobuf first, then grpc
ENV GRPC_RELEASE_TAG=v1.71.0
RUN git clone --recurse-submodules -b v1.71.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
RUN	cd grpc && mkdir -p cmake/build && cd cmake/build && \
  cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -S ../.. && \
  make -j 4 && \
  make install

COPY . .

RUN ls
# -DgRPC_SSL_PROVIDER=OpenSSL
# ENV LD_LIBRARY_PATH=/.local/lib
# RUN apt-get install openssl
# RUN apt-get install libssl-dev
ENV DISPATCHER_ADDRESS=localhost:50501
RUN cd src/cloud && rm CMakeCache.txt && cmake -DgRPC_SSL_PROVIDER=OpenSSL -DCMAKE_INSTALL_PREFIX=%MY_INSTALL_DIR% . && make -j 4

EXPOSE 9090

CMD /src/cloud/crawler_wrapper.sh
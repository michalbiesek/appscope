FROM alpine:latest

# RUN apk add bash binutils cmake cgdb curl gcc g++ grpc-dev re2-dev protobuf-dev c-ares-dev git make
RUN apk add autoconf bash binutils cmake cgdb curl gcc g++ re2-dev protobuf-dev c-ares-dev git make libtool linux-headers

RUN mkdir -p /opt/telemetry-demo && \
    cd /opt/telemetry-demo && \
    git clone https://github.com/open-telemetry/opentelemetry-demo.git .

ARG OPENTELEMETRY_CPP_VERSION=1.9.0

RUN git clone --recurse-submodules -b v1.52.2 --depth 1 --shallow-submodules https://github.com/grpc/grpc \
	&& cd grpc \
	&& mkdir -p cmake/build \
	&& cd cmake/build \
    && cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_BUILD_TYPE=Debug \
      ../.. \ 
	&&  make -j$(nproc) \
	&&  make install \
	&&  cd ../../..

RUN mkdir -p /opt/opentelemetry-cpp \
	&& cd /opt/opentelemetry-cpp/ \
     && git clone https://github.com/open-telemetry/opentelemetry-cpp . \
    	&& git checkout tags/v${OPENTELEMETRY_CPP_VERSION} -b v${OPENTELEMETRY_CPP_VERSION} \
	&& mkdir build \
	&& cd build \
	&& cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
			  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF \
			  -DWITH_EXAMPLES=OFF -DWITH_OTLP=ON -DWITH_OTLP_GRPC=ON \
	&& make -j$(nproc) install && cd ../..

RUN cd  /opt/telemetry-demo/src/currencyservice/ \
    && mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. \
    && make -j$(nproc) install

ENV SCOPE_CRIBL_ENABLE=false

ENV PATH="/usr/local/scope:/usr/local/scope/bin:${PATH}"
RUN echo "export PATH=/usr/local/scope:/usr/local/scope/bin:${PATH}" >/etc/profile.d/path.sh

COPY scope-profile.sh /etc/profile.d/scope.sh
COPY gdbinit /root/.gdbinit

RUN  mkdir /usr/local/scope && \
     mkdir /usr/local/scope/bin && \
     mkdir /usr/local/scope/lib && \
     mkdir -p /opt/test-runner/logs/ && \
     ln -s /opt/appscope/bin/linux/$(uname -m)/scope /usr/local/scope/bin/scope

COPY opentelemetry/scope-test /usr/local/scope/scope-test

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["test"]

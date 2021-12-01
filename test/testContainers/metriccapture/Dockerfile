FROM node

RUN mkdir /opt/metric_test
WORKDIR /opt/metric_test
RUN npm install hot-shots && \
    apt-get -o Acquire::Check-Valid-Until=false update && \
    apt-get install -y vim gawk gdb emacs lsof tcpdump wget curl netcat


COPY metriccapture/hotshot.ts /opt/metric_test/hotshot.ts

RUN  mkdir /usr/local/scope && \
     mkdir /usr/local/scope/bin && \
     mkdir /usr/local/scope/lib && \
     mkdir -p /opt/test-runner/logs/ && \
     ln -s /opt/appscope/bin/linux/$(uname -m)/scope /usr/local/scope/bin/scope && \
     ln -s /opt/appscope/bin/linux/$(uname -m)/ldscope /usr/local/scope/bin/ldscope && \
     ln -s /opt/appscope/lib/linux/$(uname -m)/libscope.so /usr/local/scope/lib/libscope.so

COPY metriccapture/scope-test /usr/local/scope/scope-test

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["test"]

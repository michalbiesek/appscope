FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y \
      binutils \
      cgdb \
      curl \
      gcc \
      gdb \
      libssl-dev \
      musl-dev \
      nano \
      netcat \
      openssl \
      python3-pip \
      strace \
      wget \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/extract_scope && \
    mkdir -p /opt/patch_libscope

ENV SCOPE_LOG_LEVEL=debug
ENV SCOPE_METRIC_VERBOSITY=4
ENV SCOPE_EVENT_LOGFILE=true
ENV SCOPE_EVENT_CONSOLE=true
ENV SCOPE_EVENT_METRIC=true
ENV SCOPE_EVENT_HTTP=true
ENV SCOPE_EVENT_NET=true
ENV SCOPE_EVENT_FS=true
ENV SCOPE_LOG_DEST=file:///opt/test-runner/logs/scope.log
ENV SCOPE_EVENT_DEST=file:///opt/test-runner/logs/events.log
ENV LC_CTYPE=C.UTF-8

ENV PATH="/usr/local/scope:/usr/local/scope/bin:${PATH}"
COPY scope-profile.sh /etc/profile.d/scope.sh
COPY gdbinit /root/.gdbinit

RUN pip3 install capstone keystone-engine unicorn ropper
RUN bash -c "$(wget http://gef.blah.cat/sh -O -)"

RUN  mkdir /usr/local/scope && \
     mkdir /usr/local/scope/bin && \
     mkdir /usr/local/scope/lib && \
     mkdir -p /opt/test-runner/logs/ && \
     ln -s /opt/appscope/bin/linux/$(uname -m)/scope /usr/local/scope/bin/scope && \
     ln -s /opt/appscope/bin/linux/$(uname -m)/ldscope /usr/local/scope/bin/ldscope && \
     ln -s /opt/appscope/lib/linux/$(uname -m)/libscope.so /usr/local/scope/lib/libscope.so

COPY glibc/scope-test /usr/local/scope/scope-test

COPY docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["test"]

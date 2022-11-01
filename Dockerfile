FROM registry.access.redhat.com/ubi8/ubi:latest
LABEL description="ctunnel"
MAINTAINER Michael Bhola <mike@nockmaar.me.uk>
ENV LogLevel "info"
RUN echo "enabled=0" >>/etc/yum/pluginconf.d/subscription-manager.conf
RUN yum install -y procps-ng nmap-ncat
COPY src/ctunnel /usr/local/bin/

CMD ctunnel -n $(awk 'BEGIN { if ("$MODE" == "SERVER") printf "-s"; else printf "-c"; }') -b ${BUFFER_SIZE:-2048} -S -z ${COMPRESSION_LEVEL:-5} -l $LISTEN_HOST:$LISTEN_PORT -f $FORWARD_HOST:$FORWARD_PORT -C ${CIPHER:-"proxy"}

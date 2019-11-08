FROM debian
RUN apt-get update
RUN apt-get upgrade
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get -y install apt-utils
RUN apt-get -y install net-tools
WORKDIR /app
COPY app .
EXPOSE 3865/udp
CMD [ "/app/xplhub", "eth0" ]

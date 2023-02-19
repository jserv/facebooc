FROM  rockylinux:9.1 AS BUILD

WORKDIR /opt/facebooc

COPY . .

RUN  yum install -yq gcc gcc-c++ glibc-devel make sqlite-devel sqlite && \ 
     make all 

FROM ubuntu:latest AS RUN
WORKDIR /opt/facebooc
COPY --from=BUILD /opt/facebooc/bin ./bin/
COPY --from=BUILD /opt/facebooc/static ./static/
COPY --from=BUILD /opt/facebooc/templates ./templates/
RUN apt-get update && apt-get install libsqlite3-0 && rm -rf /var/lib/apt/lists/*
EXPOSE 9091
CMD bin/facebooc
FROM  rockylinux:9.1

WORKDIR /opt/facebooc

COPY . .
#yum group info "Development Tools"
RUN  yum update -y &&  \
     yum install -yq gcc gcc-c++ glibc-devel make git sqlite-devel sqlite && \ 
     make all 

EXPOSE 9091

CMD bin/facebooc
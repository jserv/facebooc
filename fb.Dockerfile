FROM nhiuana/ubuntu-libsqlite3:latest
WORKDIR /opt/facebooc
COPY ./bin ./bin/
COPY ./static ./static/
COPY ./templates ./templates/
EXPOSE 9091
CMD bin/facebooc
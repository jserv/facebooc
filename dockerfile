FROM gcc

RUN mkdir -p /app

RUN apt update

RUN apt install -y libsqlite3-dev

# RUN apt install -y -q build-essential curl

WORKDIR /app

COPY ./ /app/

RUN make all

EXPOSE 8080

CMD [ "./bin/facebooc" ]
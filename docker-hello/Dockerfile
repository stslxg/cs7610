FROM ubuntu:latest

RUN apt-get update
RUN apt-get install -y gcc

ADD hello.c /app/
WORKDIR /app/
RUN gcc hello.c -o hello

ENTRYPOINT /app/hello


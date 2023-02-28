FROM ubuntu:18.04

RUN apt-get update
RUN apt-get install -qmy g++ cmake netcat iputils-ping

COPY . /app
WORKDIR /app
RUN make

ENTRYPOINT ["/app/prj2"]
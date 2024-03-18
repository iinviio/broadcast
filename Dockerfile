FROM gcc:latest as builder
COPY src /usr/src
WORKDIR /usr/src
RUN gcc -o main -static main.c

FROM ubuntu:latest as runtime
COPY --from=builder /usr/src/main /main
CMD ["./main"]



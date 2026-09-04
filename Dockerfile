FROM alpine:3.19 AS builder

RUN apk add --no-cache gcc make musl-dev
WORKDIR /build
COPY . .
RUN make release

FROM alpine:3.19
RUN apk add --no-cache gcc nasm musl-dev bash
COPY --from=builder /build/ccompiler /usr/local/bin/ccompiler
COPY buildasm.sh /usr/local/bin/buildasm

WORKDIR /workspace
ENTRYPOINT ["ccompiler"]
CMD ["--help"]

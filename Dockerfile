FROM buildpack-deps:focal as BUILDER

WORKDIR /app

RUN apt-get update && apt-get install -y \
    cmake \
    yasm \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://gitlab.com/AOMediaCodec/SVT-AV1.git && \
    cd ./SVT-AV1/Build/linux && \
    ./build.sh release



FROM ubuntu:focal as RELEASE

WORKDIR /app

ENV PATH "$PATH:/app"
ENV LD_LIBRARY_PATH "/app"

COPY --from=BUILDER /app/SVT-AV1/Bin/Release/ /app/

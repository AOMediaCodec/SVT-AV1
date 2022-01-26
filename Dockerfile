FROM alpine as BUILDER

WORKDIR /app

RUN apk add \
    build-base \
    cmake \
    yasm \
    git

ENV LDFLAGS "-static -static-libgcc"

RUN git clone --depth 1 https://gitlab.com/AOMediaCodec/SVT-AV1.git && \
    cd ./SVT-AV1/Build/linux && \
    ./build.sh release static



FROM alpine as RELEASE

COPY --from=BUILDER /app/SVT-AV1/Bin/Release/SvtAv1EncApp /app/

ENTRYPOINT /app/SvtAv1EncApp

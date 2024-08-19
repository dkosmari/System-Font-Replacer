FROM devkitpro/devkitppc

COPY --from=ghcr.io/wiiu-env/libmocha:20240603 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20240505 /artifacts $DEVKITPRO

RUN apt-get install -y automake

COPY . /project
WORKDIR /project

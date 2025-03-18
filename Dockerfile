FROM devkitpro/devkitppc

COPY --from=ghcr.io/wiiu-env/libmocha:20240603 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:0.8.2-dev-20241128-1ac579a /artifacts $DEVKITPRO

RUN apt-get install -y automake
RUN dkp-pacman -Syu --noconfirm

COPY . /project
WORKDIR /project

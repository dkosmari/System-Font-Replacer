# FROM devkitpro/devkitppc
FROM ghcr.io/wiiu-env/devkitppc:20241128

COPY --from=ghcr.io/wiiu-env/libmocha:20240603 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20250208 /artifacts $DEVKITPRO

RUN apt-get install -y automake
# RUN dkp-pacman -Syu --noconfirm

COPY . /project
WORKDIR /project

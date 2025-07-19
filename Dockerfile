# FROM devkitpro/devkitppc
FROM dkosmari/devkitppc-wiiu-debian

COPY --from=ghcr.io/wiiu-env/libmocha:20250608 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20250608 /artifacts $DEVKITPRO

RUN apt-get install -y automake
# RUN dkp-pacman -Syu --noconfirm

COPY . /project
WORKDIR /project

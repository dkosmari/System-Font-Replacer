FROM devkitpro/devkitppc
# FROM dkosmari/devkitppc-wiiu-debian

RUN apt-get install -y automake libtool
RUN dkp-pacman -Syu --noconfirm

COPY . /project
WORKDIR /project

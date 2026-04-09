FROM devkitpro/devkitppc

RUN apt-get install -y automake libtool
RUN dkp-pacman -Syu --noconfirm

COPY . /project
WORKDIR /project

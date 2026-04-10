# FROM devkitpro/devkitppc
FROM dkosmari/devkitppc-wiiu-debian

# RUN apt-get install -y automake libtool
# RUN dkp-pacman -Syu --noconfirm

USER user

WORKDIR /project
COPY --chown=user:user . /project

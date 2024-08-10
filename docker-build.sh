#!/bin/bash

PLUGIN=system-font-replacer
IMAGE=aroma-plugin-$PLUGIN-image
CONTAINER=aroma-plugin-$PLUGIN-container

cleanup()
{
    echo "Cleaning up Docker..."
    if test x$CONTAINER != x
    then
        docker container rm --force $CONTAINER
    fi
    if test x$IMAGE != x
    then
        docker image rm --force $IMAGE
    fi
    echo "You may also want to run 'docker system prune --force' to delete Docker's caches."
    exit $1
}
trap cleanup INT TERM

docker build --tag $IMAGE . || cleanup 1

ARGS="--tty --interactive --name $CONTAINER $IMAGE"
docker run $ARGS sh -c "./bootstrap && ./configure --host=powerpc-eabi CXXFLAGS='-O2 -ffunction-sections -fipa-pta' && make" || cleanup 2
echo "Compilation finished."

# Copy the wps file out.
docker cp "$CONTAINER:/project/$PLUGIN.wps" .

cleanup 0

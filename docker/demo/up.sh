#!/usr/bin/env bash
xhost +
if docker ps -a | grep -q demo
then
	docker start demo && docker exec -it demo /bin/bash -l
else
	docker run --name=demo -it --privileged  -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/dri:/dev/dri -v /dev/input:/dev/input -v /dev/uinput:/dev/uinput -v /sys/devices:/sys/devices -v /home/$SUDO_USER/demo:/home/dev/demo "$@" demo /bin/bash -l
fi

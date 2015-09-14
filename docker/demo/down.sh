#!/usr/bin/env bash
xhost -
if docker ps | grep -q demo
then
	docker stop demo
fi
if docker ps -a | grep -q demo
then
	docker rm demo
fi

FROM debian:jessie

ENV DEBIAN_FRONTEND=noninteractive
ADD sources.list /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y \
	build-essential \
	cmake \
	pkg-config \
	python \
	python-setuptools \
	python3 \
	python3-setuptools
RUN apt-get install -y \
	git \
	screen \
	strace \
	sudo \
	tmux \
	vim \
	wget
RUN easy_install -U pip && easy_install3 -U pip

RUN adduser --disabled-password --gecos "Developer,,," dev && \
	usermod -aG sudo dev
ADD sudoers /etc/sudoers
RUN mkdir -p /home/dev/bin /home/dev/lib /home/dev/include && \
	chown -R dev: /home/dev

ENV PATH /home/dev/bin:$PATH
ENV PKG_CONFIG_PATH /home/dev/lib/pkgconfig
ENV LD_LIBRARY_PATH /home/dev/lib

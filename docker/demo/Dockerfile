# Common 
FROM dev

ENV DEBIAN_FRONTEND=noninteractive
ADD sources.list /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y mesa-utils xserver-xorg-video-all
RUN apt-get install -y libglfw3-dev
RUN apt-get install -y python-opencv python-opengl python-xlib
# To compile the Python package
RUN apt-get install -y libudev-dev
RUN pip2 install -U glfw 

# Useful for some 3D games (i.e. Sauerbraten)
RUN apt-get install -y libsdl-mixer1.2 libsdl-image1.2

ENV HOME /home/dev
WORKDIR /home/dev
USER dev

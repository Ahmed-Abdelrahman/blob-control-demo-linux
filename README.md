This is a (very) rudimentary demonstration of mapping 2D greyscale pressure pad
array data to key input and passing it to an X window.

You'll want Docker to run this demo. On Debian Jessie, if you have backports
enabled, do

	apt-get install docker.io

Then, build the containers in `docker/dev` and `docker/demo`:

	cd docker/dev
	docker build -t dev .
	cd ../demo
	docker build -t demo .

From there, use the `up.sh` and `down.sh` scripts to create/enter and destroy
(respectively) the container. You can pass additional arguments to `docker run`
via `up.sh`. For example, to mount a directory with a 3D game,

	./up.sh -v /home/user/sauerbraten:/home/dev/sauerbraten

To run the demo, use `xwininfo` to get the X window ID of the target
application after it's running, then pass that as the first parameter to
`demo.py`. Then click around to hit the `wasd` keys. `Escape` is bound to
`Enter`; ideally all keyboard input would go to the target window. Beware: this
won't work if a game expects to have focus.

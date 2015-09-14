#!/usr/bin/env python2

import ctypes
import sys
import time

import numpy as np

import Xlib
import Xlib.X
import Xlib.display
from Xlib.keysymdef import latin1, miscellany
from Xlib.protocol.event import KeyPress, KeyRelease

import glfw
import OpenGL.GL as gl

import cv2

# Settings

WINDOW_WIDTH = 640
WINDOW_HEIGHT = 640

# Callbacks and global variables

# GLFW window
window = None

# OpenGL pointers
program = None
fbo = None
rb = None
pb = None

# Image data, RGBA8 format, bottom-left coordinates
# Note that OpenCV expects BGRA8 and top-left coordinates
image = None

blob_pos = (0.0, 0.0)

mouse_down = 0
cursor_posx = 0.0
cursor_posy = 0.0

# X11 junk
display = Xlib.display.Display()
dest_window = None
charkeys = {
        'w': display.keysym_to_keycode(latin1.XK_w),
        'a': display.keysym_to_keycode(latin1.XK_a),
        's': display.keysym_to_keycode(latin1.XK_s),
        'd': display.keysym_to_keycode(latin1.XK_d),
        'Escape': display.keysym_to_keycode(miscellany.XK_Escape)
        }

old_keys = []

# Blob detector instance
params = cv2.SimpleBlobDetector_Params()
params.blobColor = 255
detector = cv2.SimpleBlobDetector(params)

def render():
    global image

    # Draw to the offscreen FBO
    gl.glBindFramebuffer(gl.GL_DRAW_FRAMEBUFFER, fbo)
    gl.glDrawBuffer(gl.GL_COLOR_ATTACHMENT0)

    gl.glClear(gl.GL_COLOR_BUFFER_BIT)
    gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4)

    # Start to copy off to the pixel buffer; this will continue
    # asynchronously with frame delay
    gl.glBindBuffer(gl.GL_PIXEL_PACK_BUFFER, pb)
    # Yes, you have to do glBufferData every time
    gl.glBufferData(gl.GL_PIXEL_PACK_BUFFER, WINDOW_WIDTH * WINDOW_HEIGHT * 4,
            ctypes.c_void_p(0), gl.GL_DYNAMIC_READ)
    gl.glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, gl.GL_RGBA,
            gl.GL_UNSIGNED_BYTE, ctypes.c_void_p(0))

    # Copy from offscreen framebuffer to display framebuffer
    gl.glBindFramebuffer(gl.GL_READ_FRAMEBUFFER, fbo)
    gl.glBindFramebuffer(gl.GL_DRAW_FRAMEBUFFER, 0)
    gl.glReadBuffer(gl.GL_COLOR_ATTACHMENT0)
    gl.glDrawBuffer(gl.GL_BACK_LEFT)

    gl.glClear(gl.GL_COLOR_BUFFER_BIT)
    gl.glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, WINDOW_WIDTH,
            WINDOW_HEIGHT, gl.GL_COLOR_BUFFER_BIT, gl.GL_NEAREST)

    # Now actually map to the pixel buffer and copy image data
    image_data_p = ctypes.c_void_p(gl.glMapBuffer(gl.GL_PIXEL_PACK_BUFFER,
            gl.GL_READ_ONLY))
    image_data_ubyte_p = ctypes.cast(image_data_p,
            ctypes.POINTER(ctypes.c_ubyte))
    image = np.ctypeslib.as_array(image_data_ubyte_p, shape=(WINDOW_WIDTH,
            WINDOW_HEIGHT, 4))
    gl.glUnmapBuffer(gl.GL_PIXEL_PACK_BUFFER)

def mark_blob():
    global blob_pos
    keypoints = detector.detect(image[:,:,0])
    if len(keypoints) > 0:
        blob_pos = keypoints[0].pt
    else:
        blob_pos = None

def keyev(char, press):
    if press:
        return KeyPress(
                time=int(time.time()),
                root=0,
                window=dest_window,
                same_screen=0,
                child=Xlib.X.NONE,
                root_x=0,
                root_y=0,
                event_x=0,
                event_y=0,
                state=0,
                detail=charkeys[char],
                )
    else:
        return KeyRelease(
                time=int(time.time()),
                root=0,
                window=dest_window,
                same_screen=0,
                child=Xlib.X.NONE,
                root_x=0,
                root_y=0,
                event_x=0,
                event_y=0,
                state=0,
                detail=charkeys[char],
                )

# Use X11 to  throw input to the target window
def do_input():
    global old_keys

    new_keys = []

    # Map cursor position to key presses
    if blob_pos is not None:
        if blob_pos[0] > 2.0 / 3.0 * WINDOW_WIDTH:
            new_keys.append('d')
        elif blob_pos[0] < 1.0 / 3.0 * WINDOW_WIDTH:
            new_keys.append('a')
        if blob_pos[1] > 2.0 / 3.0 * WINDOW_HEIGHT:
            new_keys.append('w')
        elif blob_pos[1] < 1.0 / 3.0 * WINDOW_HEIGHT:
            new_keys.append('s')

    if glfw.get_key(window, glfw.KEY_ENTER) == glfw.PRESS:
        new_keys.append('Escape')

    # Send press and release events when key state changes
    for char in ['w', 'a', 's', 'd', 'Escape']:
        new = char in new_keys
        old = char in old_keys
        if new:
            if not old:
                display.send_event(dest_window, keyev(char, True),
                        event_mask=1)
        elif old:
            if not new:
                display.send_event(dest_window, keyev(char, False),
                        event_mask=1)

    # Events don't get processed unless they are manually flushed
    while display.pending_events() > 0:
        display.poll_events()

    old_keys = new_keys

def update_cursor(window, posx, posy):
    global cursor_posx
    global cursor_posy
    cursor_posx = posx
    cursor_posy = posy

def update_mouse_btn(window, button, action, mods):
    global mouse_down
    if button == glfw.MOUSE_BUTTON_LEFT:
        if action == glfw.PRESS:
            mouse_down = 1
        else:
            mouse_down = 0

def glfw_init_routine():
    global window

    if not glfw.init():
        print("Failed to init")
        return False

    glfw.window_hint(glfw.RESIZABLE, 0)
    window = glfw.create_window(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello, world!",
            None, None)
    if not window:
        print("Failed to create window")
        glfw.terminate()
        return False

    glfw.set_cursor_pos_callback(window, update_cursor)
    glfw.set_mouse_button_callback(window, update_mouse_btn)

    glfw.make_context_current(window)

    return True

def gl_init_routine():
    global program
    global fbo
    global rb
    global pb
    global image

    # Compile and link shaders

    vert_shader = gl.glCreateShader(gl.GL_VERTEX_SHADER)
    frag_shader = gl.glCreateShader(gl.GL_FRAGMENT_SHADER)

    with open('surface_vert.glsl') as f:
        vert_shader_text = f.read()
    with open('surface_frag.glsl') as f:
        frag_shader_text = f.read()

    gl.glShaderSource(vert_shader, vert_shader_text)
    gl.glShaderSource(frag_shader, frag_shader_text)

    gl.glCompileShader(vert_shader)
    gl.glCompileShader(frag_shader)

    if not gl.glGetShaderiv(vert_shader, gl.GL_COMPILE_STATUS):
        print("Vertex shader did not compile")
        return False

    if not gl.glGetShaderiv(frag_shader, gl.GL_COMPILE_STATUS):
        print("Fragment shader did not compile")
        return False

    program = gl.glCreateProgram()

    gl.glAttachShader(program, vert_shader)
    gl.glAttachShader(program, frag_shader)
    gl.glLinkProgram(program)

    gl.glDetachShader(program, vert_shader)
    gl.glDetachShader(program, frag_shader)

    gl.glUseProgram(program)

    if not gl.glGetProgramiv(program, gl.GL_LINK_STATUS):
        print("Shader not linked")
        return False

    # Create and bind buffers

    # Vertex data buffer

    vert_dtype = np.dtype([('position', np.float32, 2)])
    data = np.zeros(4, dtype=vert_dtype)
    data['position'] = [(-1, -1), (-1, 1), (1, -1), (1, 1)]
    stride = data.strides[0]

    # vb is the vertex buffer
    vb, pb = gl.glGenBuffers(2)
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vb)
    gl.glBufferData(gl.GL_ARRAY_BUFFER, data.nbytes, data, gl.GL_DYNAMIC_DRAW)

    offset = ctypes.c_void_p(0)
    loc = gl.glGetAttribLocation(program, 'position')
    gl.glEnableVertexAttribArray(loc)
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vb)
    gl.glVertexAttribPointer(loc, 2, gl.GL_FLOAT, False, stride, offset)

    # Offscreen framebuffer for OpenCV

    fbo = gl.glGenFramebuffers(1)
    rb = gl.glGenRenderbuffers(1)
    gl.glBindRenderbuffer(gl.GL_RENDERBUFFER, rb)
    gl.glRenderbufferStorage(gl.GL_RENDERBUFFER, gl.GL_RGBA8, WINDOW_WIDTH,
            WINDOW_HEIGHT)

    gl.glBindFramebuffer(gl.GL_DRAW_FRAMEBUFFER, fbo)
    gl.glFramebufferRenderbuffer(gl.GL_DRAW_FRAMEBUFFER,
            gl.GL_COLOR_ATTACHMENT0, gl.GL_RENDERBUFFER, rb)

    return True

def gl_deinit_routine():
    pass

def update_surface():
    clicked_loc = gl.glGetUniformLocation(program, 'mouse_down')
    cursor_pos_loc = gl.glGetUniformLocation(program, 'cursor_pos')
    gl.glUniform1i(clicked_loc, mouse_down)
    gl.glUniform2f(cursor_pos_loc, cursor_posx, WINDOW_HEIGHT - cursor_posy)

def main():
    if not glfw_init_routine():
        return

    if not gl_init_routine():
        return

    # Main event loop
    while not glfw.window_should_close(window):
        update_surface()
        render()
        glfw.swap_buffers(window)
        mark_blob()
        do_input()
        glfw.poll_events()

    gl_deinit_routine()

    glfw.terminate()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python demo.py X11_WINDOW_ID")
        exit(0)

    dest_window = display.create_resource_object('window', int(sys.argv[1], 0))

    main()

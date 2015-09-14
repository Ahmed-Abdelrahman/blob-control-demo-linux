uniform bool mouse_down;
uniform vec2 cursor_pos;

void main() {
	if (mouse_down) {
		vec2 d = gl_FragCoord.xy - cursor_pos;
		float r2 = d.x * d.x + d.y * d.y;
		float level = 2.0 / (exp(r2 / 1024.0) + exp(r2 / -1024.0));

		gl_FragColor = vec4(level, level, level, 1.0);
	} else {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}

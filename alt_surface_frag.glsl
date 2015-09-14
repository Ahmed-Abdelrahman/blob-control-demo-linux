uniform bool mouse_down;
uniform vec2 cursor_pos;

const vec4 blue = vec4(0.0, 0.5, 1.0, 1.0);
const vec4 cyan = vec4(0.5, 1.0, 0.5, 1.0);
const vec4 yellow = vec4(1.0, 0.5, 0.0, 1.0);
const vec4 red = vec4(0.5, 0.0, 0.0, 1.0);

void main() {
	if (mouse_down) {
		vec2 d = gl_FragCoord.xy - cursor_pos;
		float r2 = d.x * d.x + d.y * d.y;
		float level = 2.0 / (exp(r2 / 1024.0) + exp(r2 / -1024.0));

		vec4 color = blue;
		if (level < 1.0 / 3.0) {
			float t = 3.0 * level;
			color = (1.0 - t) * blue + t * cyan;
		} else if (level < 2.0 / 3.0) {
			float t = 3.0 * level - 1.0;
			color = (1.0 - t) * cyan + t * yellow;
		} else {
			float t = 3.0 * level - 2.0;
			color = (1.0 - t) * yellow + t * red;
		}

		gl_FragColor = color;
	} else {
		gl_FragColor = blue;
	}
}

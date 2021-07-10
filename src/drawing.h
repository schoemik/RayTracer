struct DrawBuffer {
	u32 width;
	u32 height;
	void* memory;
};


static void
fillBuffer(DrawBuffer* buffer, u32 color) {
	u32* p = (u32*)buffer->memory;
	for (int y = 0; y < buffer->height; y++) {
		for (int x = 0; x < buffer->width; x++) {
			*p++ = color;
		}
	}
}

static void
drawPoint(DrawBuffer* buffer, int x, int y, u32 color) {
	u32* p = (u32*)buffer->memory + (((y * buffer->width) + x));
	if((u32*)buffer->memory < p && p < (u32*)buffer->memory + buffer->width * buffer->height) {
		*p = color;
	}
}

static void
drawLine(DrawBuffer* buffer, V2 vec1, V2 vec2, u32 color) {
	float dx = vec2.x - vec1.x;
	float dy = vec2.y - vec1.y;
	if(dx < 0) { drawLine(buffer, vec2, vec1, color); return; }
	if(dy == 0) { drawLine(buffer, vec2, vec1, color); return; }
	if(dx >= dy) {
		for(float i = 0; i < dx; i++) {
			float step = i/dx;
			if(vec1.x + i < buffer->width && vec1.y + step * dy < buffer->height) {
				drawPoint(buffer, (int)(vec1.x + i), (int)(vec1.y + step * dy), color);
			}
		}
	} else {
		for(float i = 0; i < dy; i++) {
			float step = i/dy;
			if(vec1.x + step * dx < buffer->width && vec1.y + i < buffer->height) {
				drawPoint(buffer, (int)(vec1.x + step * dx), (int)(vec1.y + i), color);
			}
		} 
	}
}

static void
drawTriangle(DrawBuffer* buffer, V2 vec1, V2 vec2, V2 vec3, u32 color) {
	drawLine(buffer, vec1, vec2, color);
	drawLine(buffer, vec2, vec3, color);
	drawLine(buffer, vec3, vec1, color);
}

static void
drawRectangle(DrawBuffer* buffer, V2 origin, V2 dim, u32 color) {
	V2 bot = {origin.x, origin.y + dim.y};
	V2 right = {origin.x + dim.x, origin.y};
	V2 botRight = {origin.x + dim.x, origin.y + dim.y};
	drawLine(buffer, origin, bot, color);
	drawLine(buffer, bot, botRight, color);
	drawLine(buffer, botRight, right, color);
	drawLine(buffer, right, origin, color);
}
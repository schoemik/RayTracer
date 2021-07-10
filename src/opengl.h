static void 
printGLError() {
	switch(glGetError()) {
		case GL_INVALID_ENUM: 		{ puts("ERROR GL_INVALID_ENUM"); return; }
		//case GL_INVALID_NAME: 		{ printf("ERROR GL_INVALID_NAME"); return; }
		case GL_INVALID_OPERATION: 	{ puts("ERROR GL_INVALID_OPERATION"); return; }
		case GL_NO_ERROR: 			{ return; }
		case GL_STACK_OVERFLOW: 	{ puts("ERROR GL_STACK_OVERFLOW"); return; }
		case GL_STACK_UNDERFLOW:	{ puts("ERROR GL_STACK_UNDERFLOW"); return; }
		case GL_OUT_OF_MEMORY: 		{ puts("ERROR GL_OUT_OF_MEMORY"); return; }
	}
}
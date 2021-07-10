#pragma pack(push)
#pragma pack(1)
struct BMPHeader {
	u16 type;
	u32 size;
	u16 r1;
	u16 r2;
	u32 offset;
	u32 headerSize;
	s32 width;
	s32 height;
	u16 numPlanes;
	u16 bitsPerPixel;
	u32 compression;
	u32 imageSize;
	s32 pixelPerMeterX;
	s32 pixelPerMeterY;
	u32 numColors;
	u32 importantColors;
};
#pragma pack(pop)

static void
rgba2bgraBuffer(DrawBuffer* buffer) {
	u32* p = (u32*)buffer->memory;
	for (int y = 0; y < buffer->height; y++) {
		for (int x = 0; x < buffer->width; x++) {
			*p++ = bgraPack4x8(rgbaUnpack4x8(*p));
		}
	}
}

inline void
writeBufferAsBitmap(DrawBuffer* buffer, LPCSTR filename) {
	HANDLE fileHandle = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(fileHandle == INVALID_HANDLE_VALUE) {
		printf("Error: Couldn't open file %s.", filename);
		return;
	}
	u32 imageSize = sizeof(u32) * buffer->width * buffer->height;
	BMPHeader header = {0};
	header.type = 0x4D42;
	header.size = sizeof(BMPHeader) + imageSize;
	header.r1 = 0;
	header.r2 = 0;
	header.offset = sizeof(BMPHeader);
	header.headerSize = 40;
	header.width = buffer->width;
	header.height = buffer->height;
	header.imageSize = imageSize;
	header.numPlanes = 1;
	header.bitsPerPixel = 32;
	header.compression = 0;
	header.pixelPerMeterX = 0;
	header.pixelPerMeterY = 0;
	header.numColors = 0;
	header.importantColors = 0;
	
	DWORD bytesWritten;
	if(!WriteFile(fileHandle, &header, sizeof(BMPHeader), &bytesWritten, 0)) {
		printf("Error while writing header.\n");
		return;
	}
	rgba2bgraBuffer(buffer);
	if(!WriteFile(fileHandle, buffer->memory, imageSize, &bytesWritten, 0)) {
		DWORD lastError = GetLastError();
		printf("Error while writing buffer. Code=%u\n", lastError);
	}
	rgba2bgraBuffer(buffer);
	CloseHandle(fileHandle);
	printf("Written file %s\n", filename);
}

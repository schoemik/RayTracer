struct V2 {
	float x;
	float y;
};

struct V3 {
	float x;
	float y;
	float z;
};

inline V3 cross(V3 vec1, V3 vec2) {
	V3 result = {
		vec1.y * vec2.z - vec1.z * vec2.y,
		vec1.z * vec2.x - vec1.x * vec2.z,
		vec1.x * vec2.y - vec1.y * vec2.x
	};
	return(result);
}

inline V3
sqrt(V3 vec) {
	V3 result = {
		(float)sqrt(vec.x),
		(float)sqrt(vec.y),
		(float)sqrt(vec.z)
	};
	return(result);
}

inline V3 
hadamard(V3 vec1, V3 vec2) {
	V3 result = {
		vec1.x * vec2.x,
		vec1.y * vec2.y,
		vec1.z * vec2.z
	};
	return(result);
}

inline 
float length(V3 v) {
	return(sqrt(v.x*v.x+v.y*v.y+v.z*v.z)); 
}

inline float sqLength(V3 v) {
	return(v.x*v.x+v.y*v.y+v.z*v.z);
}

inline 
V3 norm(V3 v) { 
	float l = length(v); 
	V3 result = {v.x/l, v.y/l, v.z/l};
	return(result); 
}

inline 
float dot(V3 vec1, V3 vec2) {
	return(vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z);
}

inline 
V3 lerp(V3 vec1, V3 vec2, float t) {
	V3 result = {
		(1.f - t) * vec1.x + t * vec2.x,
		(1.f - t) * vec1.y + t * vec2.y,
		(1.f - t) * vec1.z + t * vec2.z
	};
	return(result);
}

inline
V3 operator+(V3 vec1, V3 vec2) {
	V3 result = {vec1.x + vec2.x, vec1.y + vec2.y, vec1.z + vec2.z};
	return(result);
}

inline V3 operator+=(V3 &vec1, V3 vec2) {
	vec1 = vec1 + vec2;
	return vec1;
}

inline
V3 operator-(V3 vec1, V3 vec2) {
	V3 result = {vec1.x - vec2.x, vec1.y - vec2.y, vec1.z - vec2.z};
	return(result);
}

inline
V3 operator-(V3 vec1) {
	V3 result = {-vec1.x, -vec1.y, -vec1.z};
	return(result);
}

inline 
V3 operator*(V3 v, float t) {
	V3 result = {
			v.x * t,
			v.y * t,
			v.z * t
		};
	return(result);
}

inline V3
operator*(float t, V3 v) {
	return(v*t);
}

inline
V3 operator/(V3 v, float t) {
	float inv = 1 / t;
	return(v * inv);
}

inline u32
xOrShift32(u32* state) {
	u32 x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return(x);
}

inline
f32 randomFloat(u32 *series) {
	return((f32)(xOrShift32(series) / (f32)U32Max));
}

inline 
V3 randomVector(u32* series) {
	V3 result = { randomFloat(series), randomFloat(series), randomFloat(series) };
	return(result);
}

inline V3
v3(float x, float y, float z) {
	V3 result;
	result.x = x;
	result.y = y;
	result.z = z;
	return(result);
}

inline
V3 randomUnitVector(u32* series) {
	V3 p = {0.0f, 0.0f, 0.0f};
	V3 p1 = { 1.0f, 1.0f, 1.0f };
	do {
		p = (2.0f * randomVector(series) - p1);
	} while (sqLength(p) > 1.0f);
	return(p);
}

inline
V3 randomUnitDisc(u32* series) {
	V3 p = {0};
	do {
		p = 2.0f * v3(randomFloat(series), randomFloat(series),  0.0f) - v3(1.0f, 1.0f, 0.0f);
	} while(dot(p,p) >= 1.0f);
	return(p);
}


// TODO(mike): clamping of floats?
inline 
f32 roundU32ToF32(u32 u) {
	return((f32)u/255.0f);
}

inline 
u32 roundFloatToU32(f32 f) {
	return((u32)((f) * 255.0f));
}

struct V4 {
	float x;
	float y;
	float z;
	float w;
};

inline V4
v4(f32 x, f32 y, f32 z, f32 w) {
	V4 result = {x,y,z,w};
	return(result);
}

inline
V4 bgraUnpack4x8(u32 packed) {
	V4 result = {
		roundU32ToF32((packed & 0x00ff0000) >> 16),
		roundU32ToF32((packed & 0x0000ff00) >> 8),
		roundU32ToF32((packed & 0x000000ff) >> 0),
		roundU32ToF32((packed & 0xff000000) >> 24),
	};
	return(result);
}

inline
V4 rgbaUnpack4x8(u32 packed) {
	V4 result = {
		roundU32ToF32((packed & 0x000000ff) >> 0),
		roundU32ToF32((packed & 0x0000ff00) >> 8),
		roundU32ToF32((packed & 0x00ff0000) >> 16),
		roundU32ToF32((packed & 0xff000000) >> 24),
	};
	return(result);
}

inline
u32 bgraPack4x8(V4 unpacked) {
	u32 result = (
		(roundFloatToU32(unpacked.x) << 16) |
		(roundFloatToU32(unpacked.y) << 8) |
		(roundFloatToU32(unpacked.z) << 0) |
		(roundFloatToU32(unpacked.w) << 24)
	);
	return(result);
}

inline
u32 rgbaPack4x8(V4 unpacked) {
	u32 result = (
		(roundFloatToU32(unpacked.x) << 0) |
		(roundFloatToU32(unpacked.y) << 8) |
		(roundFloatToU32(unpacked.z) << 16) |
		(roundFloatToU32(unpacked.w) << 24)
	);
	return(result);
}

inline u8 
refract(V3 vec, V3 normal, f32 ni_over_nt, V3* refracted) {
	V3 uv = norm(vec);
	f32 dt = dot(uv, normal);
	f32 discriminant = 1.0f - ni_over_nt*ni_over_nt*(1.0f-dt*dt);
	if(discriminant > 0) {
		*refracted = ni_over_nt*(uv - normal*dt) - normal*(f32)sqrt(discriminant);
		return(true);
	}
	return false;
}

inline V3
reflect(V3 vec1, V3 vec2) {
	return(vec2 - 2*dot(vec1, vec2)*vec1);
}

inline f32 
schlick(f32 cos, f32 ref) {
	f32 r0 = (1 - ref) / (1 + ref);
	r0 = r0 * r0;
	return(r0 + (1 - r0) * (f32)pow((1 - cos), 5));
}


inline f32
clamp(f32 val, f32 min, f32 max) {
	if(val <= min) {
		return(min);
	} else if(val >= max) {
		return(max);
	}
	return(val);
}

inline 
f32 LinearTosRGB(f32 l) {
	if(l < 0.0f) {
		l = 0.0f;
	}
	if(l > 1.0f) {
		l = 1.0f;
	}
	f32 s = l*12.92f;;
	if(l > 0.0031308f) {
		s = 1.055f * pow(l, 1.0f/2.4f) - 0.055f;
	}
	return(s);
}	
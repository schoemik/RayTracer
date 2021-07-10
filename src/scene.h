struct Ray {
	V3 ori;
	V3 dir;
};

inline
Ray ray(V3 o, V3 d) {
	Ray result = {o,d};
	return(result);
}

inline 
V3 pointOnRay(Ray r, f32 t) {
	return(r.ori + (r.dir * t));
}

struct HitRecord {
	f32 t;
	V3 pos;
	V3 normal;
	u32 matIndex;
};

struct Sphere {
	V3 pos;
	f32 r;
	u32 matIndex;
};

inline  
bool hitSphere(Sphere s, Ray ray, f32 min, f32 max, HitRecord* rec) {
	V3 oc = ray.ori - s.pos;
	f32 a = dot(ray.dir, ray.dir);
	f32 b = dot(oc, ray.dir);
	f32 c = dot(oc, oc) - s.r * s.r;
	f32 discriminant = b*b - a*c;
	if(discriminant > 0) {
		f32 temp = (-b - sqrt(discriminant)) / a;
		if(temp > min && temp < max) {
			rec->t = temp;
			rec->pos = pointOnRay(ray, rec->t);
			rec->normal = (rec->pos - s.pos) / s.r;
			rec->matIndex = s.matIndex;
			return(true);
		}
		temp = (-b + sqrt(discriminant)) / a;
		if(temp > min && temp < max) {
			rec->t = temp;
			rec->pos = pointOnRay(ray, rec->t);
			rec->normal = (rec->pos - s.pos) / s.r;
			rec->matIndex = s.matIndex;
			return(true);
		}
	}
	return(false);
}


struct PointLight {
	V3 color;
	V3 pos;
};

struct Material {
	V3 emission;
	V3 color;
	f32 dielectric;
	f32 metalness;
	f32 fuzz;
};

struct Camera {
	V3 origin;
	V3 lowerLeft;
	V3 horizontal;
	V3 vertical;
	V3 u;
	V3 v;
	V3 w;
	f32 lensRadius;
};

inline Camera 
camera(V3 lookFrom, V3 lookAt, V3 up, f32 fov, f32 aspect, 
		f32 aperture, f32 focusDistance) {
	Camera result;
	result.lensRadius = aperture / 2.0f;
	f32 theta = fov*M_PI/180;
	f32 halfHeight = tan(theta/2);
	f32 halfWidth = aspect * halfHeight;
	result.origin = lookFrom;
	result.w = norm(lookFrom - lookAt);
	result.u = norm(cross(up, result.w));
	result.v = cross(result.w, result.u);
	result.lowerLeft = result.origin - halfWidth*focusDistance*result.u - 
		halfHeight*focusDistance*result.v - focusDistance * result.w;
	result.horizontal = 2.0f * halfWidth * focusDistance * result.u;
	result.vertical = 2.0f * halfHeight * focusDistance * result.v;
	return(result);
}

inline 
Ray createRayFromCamera(Camera* camera, f32 s, f32 t, u32* series) {
	V3 rd = camera->lensRadius * randomUnitDisc(series);
	V3 offset = camera->u * rd.x + camera->v * rd.y;
	return(ray(camera->origin + offset, 
		camera->lowerLeft + s*camera->horizontal + t*camera->vertical - camera->origin - offset));
}


struct World {
	Camera camera;
	PointLight* lights;
	u32 lightCount;
	u8 shadowing;
	Sphere *spheres;
	u32 sphereCount;
	Material* materials;
	u32 materialCount;
};

inline
bool hitSomething(World* world, Ray r, f32 min, f32 max, HitRecord* rec) {
	HitRecord temp = {};
	bool hit_any = false;
	f32 closest = max;
	for(int i = 0; i < world->sphereCount; i++) {
		if(hitSphere(world->spheres[i], r, min, closest, &temp)) {
			hit_any = true;
			closest = temp.t;
			*rec = temp;
		}
	}
	return(hit_any);
}

inline V3 
shadow(World* world, HitRecord* rec) {
	V3 result = v3(0.001f, 0.001f, 0.001f);
	u8 firstLightHit = 0;
	u32 lightCount = 0;
	HitRecord shadowRec;
	for(int i = 0; i < world->lightCount; i++) {
		V3 lightRay = world->lights[i].pos - rec->pos;
		f32 lightRayLength = length(lightRay);
		f32 lightAttenuation = clamp(1.0f / lightRayLength, 0.0f, 1.0f);
		f32 factor = max(0.0f, dot(rec->normal, lightRay));
		V3 lightContrib = lightAttenuation * factor * world->lights[i].color;
		Ray nRay = ray(rec->pos, lightRay);
		if(!hitSomething(world, nRay, 0.001f, F32Max, &shadowRec)) {
			if(!firstLightHit) {
				firstLightHit = 1;
				lightCount = 1;
				result = lightContrib;
				continue;
			}
			lightCount++;
			result = result + lightContrib;
		} 
	}
	if(firstLightHit) {
		return(result / lightCount);
	} else {
		return(result);
	}
}

inline
V3 sampleColor(World* world, Ray r, u32 depth, u32* series) {
	HitRecord rec;
	V3 sample = v3(0.0f, 0.0f, 0.0f);
	if(hitSomething(world, r, 0.001f, F32Max, &rec)) {
		Material* mat = &world->materials[rec.matIndex];
		f32 rayLength = sqLength(rec.pos - r.ori);
		V3 attenuation = clamp(3.0f / rayLength, 0.0f, 1.0f) * v3(1.0f, 1.0f, 1.0f);
		if(mat->emission.x || mat->emission.y || mat->emission.z) {
			return(hadamard(attenuation, mat->emission));
		}
		if(depth < 8) {
			V3 diffusePath = v3(0.0f, 0.0f, 0.0f);
			V3 reflectedPath = v3(0.0f, 0.0f, 0.0f);
			Ray reflectedRay = {rec.pos, reflect(rec.normal, norm(r.dir))};
			// refract
			V3 refractedPath;
			f32 ref = mat->dielectric;
			if(ref) {
				V3 outwardNormal;
				f32 ni_over_nt;
				attenuation = v3(1.0f,1.0f,1.0f);
				V3 refracted;
				f32 reflectProb;
				f32 cos;
				f32 angle = dot(r.dir, rec.normal);
				if(angle > 0) {
					outwardNormal = -rec.normal;
					ni_over_nt = ref;
					cos = ref * angle / length(r.dir);
				} else {
					outwardNormal = rec.normal;
					ni_over_nt = 1.0f / ref;
					cos = -angle / length(r.dir);
				}
				if(refract(r.dir, outwardNormal, ni_over_nt, &refracted)) {
					reflectProb = schlick(cos, ref);
				} else {
					reflectProb = 1.0f;
				}
				if(randomFloat(series) < reflectProb) {
					refractedPath = sampleColor(world, reflectedRay, depth + 1, series);
				} else {
					refractedPath = sampleColor(world, ray(rec.pos, refracted), depth + 1, series);
				}
				return(mat->emission + refractedPath);
			}
			// diffuse 
			f32 russianRoulette = randomFloat(series);
			if(russianRoulette < (1 - mat->metalness)) {
				V3 target = rec.pos + rec.normal + randomUnitVector(series);
				Ray diffuseRay = { rec.pos, target - rec.pos };
				diffusePath = hadamard(mat->color, sampleColor(world, diffuseRay, depth + 1, series));
			} else {
				// reflect
				reflectedPath = hadamard(mat->color, sampleColor(world, reflectedRay, depth + 1, series));
			}
			return(hadamard(attenuation, diffusePath + reflectedPath));
		} else {
			return v3(0.0f, 0.0f, 0.0f);
		}
	} else {
		//return(v3(1.0f, 1.0f, 1.0f));
		return(v3(0.0f, 0.0f, 0.0f));
	}
}

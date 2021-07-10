#include <windows.h>
#include <wingdi.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define assert(expr) if(!(expr)) { char* p = 0; *p = 1; }

#ifndef DEBUG
#define DEBUG 0
#endif

typedef float f32;
typedef double f64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;  
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;  
typedef int8_t s8;

#define U32Max ((u32)-1)
#define F32Max 100000000.0f

#include "math.h"
#include "scene.h"
#include "bvh.h"
#include "drawing.h"
#include "bitmap.h"

#define WORKERCOUNT 8

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// NOTE(mike): compile with /subsystem:console to have prints to console
int 
main(int, char**) {
	return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}

static bool isRunning = true;
static int windowWidth, windowHeight, mouseX, mouseY;
static DrawBuffer graphBuffer = {};


struct Work {
	DrawBuffer* buffer;
	World* world;
	u32 sphereCount;
	u32 beginX;
	u32 beginY;
	u32 endX;
	u32 endY;
	u32 sampleCount;
	u32 series;
	u8 started;
};


HANDLE threads[WORKERCOUNT];
Work* work;
Sphere* spheres;


DWORD WINAPI
ThreadFunction(LPVOID lpParam) {
	Work work = *((Work*)lpParam);
	int sampleCount = work.sampleCount;
	for(int y = work.beginY; y < work.endY; y++) {
		u32 *pRGBA = (u32*)work.buffer->memory + 
			((y * work.buffer->width + work.beginX));
		for(int x = work.beginX; x < work.endX; x++) {
			V3 color = {0.0f, 0.0f, 0.0f};
			for(int s = 0; s < sampleCount; s++) {
				f32 u = ((f32)x + 
						randomFloat(&work.series)) / work.buffer->width;
				f32 v = ((f32)y + 
						randomFloat(&work.series)) / work.buffer->height;
				Ray r = createRayFromCamera(&work.world->camera, 
						u, v, &work.series);
				color = color + sampleColor(work.world, r, 0, &work.series);
			}
			
			color = color / sampleCount;
			color = sqrt(color);
			V4 finalColor = {
				LinearTosRGB(color.x),
				LinearTosRGB(color.y),
				LinearTosRGB(color.z),
				1.0f
			};
			*pRGBA++ = rgbaPack4x8(finalColor);
		}
	}
	return(0);
}

static u8 startThreads = 0;
static u8 pauseThreads = 0;
static u8 threadsPaused = 0;
static u8 finishedTracing = 1;

inline
void createWork(Work *w, DrawBuffer* drawBuffer, World* world,
		u32 workCount, u32 workExtraX, u32 workExtraY, u32 workWidth, u32 workHeight, u32 sampleCount) {
	u32 workX = drawBuffer->width / workWidth;
	u32 workY = drawBuffer->height / workHeight;
	u32 workRestX = drawBuffer->width % workWidth;
	u32 workRestY = drawBuffer->height % workHeight;
	Work work = {};
	work.buffer = drawBuffer;
	work.world = world;
	work.sampleCount = sampleCount;
	work.series = 453168 + workX*18241 + workY*87126;
	work.started = 0;
	for(int i = 0; i < workCount; i++) {
		u32 indexX = i % (drawBuffer->width / workWidth);
		u32 indexY = i / (drawBuffer->width / workWidth);
		work.beginX = indexX * workWidth;
		work.beginY = indexY * workHeight;
		work.endX = work.beginX + workWidth;
		work.endY = work.beginY + workHeight;
		w[i] = work;
	}
	for(int i = 0; i < workExtraX; i++) {
		work.beginX = workX * workWidth;
		work.beginY = i * workHeight;
		work.endX = work.beginX + workRestX;
		work.endY = work.beginY + workHeight;
		w[i + workCount] = work;
	}
	for(int i = 0; i < workExtraY; i++) {
		work.beginX = i * workWidth;
		work.beginY = workY * workHeight;
		work.endX = work.beginX + workWidth;
		work.endY = work.beginY + workRestY;
		w[i + workCount + workExtraX] = work;
	}
	if(workExtraX != 0 && workExtraY != 0) {
		work.beginX = workX * workWidth;
		work.beginY = workY * workHeight;
		work.endX = work.beginX + workRestX;
		work.endY = work.beginY + workRestY;
		w[workCount + workExtraX + workExtraY] = work;
	}
}

int 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE );
	const char CLASS_NAME[] = "Sample Window Class";
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = CLASS_NAME;
	
	RegisterClass(&windowClass);
	
	HWND window = CreateWindowExA(0, CLASS_NAME, "Raytracer", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if(window == NULL) {
		return(0);
	}
	
	ShowWindow(window, nShowCmd);
	HDC deviceContext = GetDC(window);
	if(deviceContext == NULL) {
		return(0);
	}
	RECT windowRect;
	GetClientRect(window, &windowRect);
	windowWidth = windowRect.right;
	windowHeight = windowRect.bottom;
	
	PIXELFORMATDESCRIPTOR pixelFormat = {};
	pixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelFormat.nVersion = 1;
	pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixelFormat.iPixelType = PFD_TYPE_RGBA;
	pixelFormat.cColorBits = 24;
	pixelFormat.cAlphaBits = 8;
	pixelFormat.iLayerType = PFD_MAIN_PLANE;
	
	//memory = (char*)VirtualAlloc(NULL, 1024 * 1024 * 100, MEM_COMMIT, PAGE_READWRITE);
	
	int nearestPixelFormat = ChoosePixelFormat(deviceContext, &pixelFormat);
	int counter = 0;
	if(nearestPixelFormat) {
		SetPixelFormat(deviceContext, nearestPixelFormat, &pixelFormat);
		HGLRC openGLRC = wglCreateContext(deviceContext);
		if(wglMakeCurrent(deviceContext, openGLRC)) {
			graphBuffer.width = windowWidth;
			graphBuffer.height = windowHeight;
			u32 bufferSize = sizeof(u32) * 4 * graphBuffer.width * graphBuffer.height; 
			graphBuffer.memory = malloc(bufferSize);

			u32 workWidth = 64;
			u32 workHeight = 64;
			u32 workRestX = graphBuffer.width % workWidth;
			u32 workRestY = graphBuffer.height % workHeight;
			u32 workCountX = graphBuffer.width / workWidth;
			u32 workCountY = graphBuffer.height / workHeight;
			u32 workExtraX = 0;
			u32 workExtraY = 0;
			u32 workExtraE = 0;
			if(workRestX != 0) { workExtraX += workCountY; }
			if(workRestY != 0) { workExtraY += workCountX; }
			if(workRestX != 0 && workRestY != 0) { workExtraE = 1; }
			u32 workCount = workCountX * workCountY;
			work = (Work*)malloc(sizeof(Work) * (workCount + workExtraX + workExtraY + workExtraE));
			
			u32 sphereCount = 9;
			static u32 sampleCount = 32;
			spheres = (Sphere*)malloc(sizeof(Sphere) * sphereCount);
			memset(spheres, 0, sizeof(Sphere) * sphereCount);
			
			u32 materialCount = 9;
			Material* materials = (Material*)malloc(sizeof(Material) * materialCount);
			memset(materials, 0, sizeof(Material) * materialCount);
			
			u32 lightCount = 1;
			PointLight* lights = (PointLight*)malloc(sizeof(PointLight) * lightCount);
			
			World* world = (World*)malloc(sizeof(World));
			
			world->spheres = spheres;
			world->materials = materials;
			V3 lookFrom = v3(0.0f, 1.0f, 10.0f);
			V3 lookAt = v3(0.0f, 1.0f, 9.0f);
			world->camera = camera(lookFrom, 
				lookAt, v3(0.0f, 1.0f, 0.0f), 
				37.8f, (f32)windowWidth/windowHeight, 0.f, length(lookFrom - lookAt));
				
			V3 lightPos = v3(3.0f, 9.0f, -3.0f);
			lights[0] = { { 1.0f, 1.0f, 1.0f }, lightPos };
			world->lights= lights;
			world->lightCount = lightCount;
		
			glBindTexture(GL_TEXTURE_2D, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graphBuffer.width, graphBuffer.height, 
				0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)graphBuffer.memory);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			
			MSG msg = {};
			clock_t beginClock;
			while(isRunning) 
			{
				while(PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				
				if(startThreads && finishedTracing) {
					printf("Ray tracing an %ux%u image with %u samples on %u threads\n", graphBuffer.width, graphBuffer.height, sampleCount, WORKERCOUNT);
					fillBuffer(&graphBuffer, rgbaPack4x8(v4(0, 0, 0, 255)));
					
					world->materialCount = materialCount;
					materials[0] = {{                    }, { 0.75f, 0.25f, 0.25f}, 0.0f, 0.0f, 0.0f};
					materials[1] = {{                    }, { 0.75f, 0.75f, 0.75f}, 0.0f, 0.0f, 0.0f};
					materials[2] = {{                    }, { 0.25f, 0.25f, 0.75f}, 0.0f, 0.0f, 0.0f};
					materials[3] = {{                    }, { 0.75f, 0.75f, 0.75f}, 0.0f, 0.0f, 0.0f};
					materials[4] = {{                    }, { 0.75f, 0.75f, 0.75f}, 0.0f, 0.0f, 0.0f};
					materials[5] = {{ 11.0f, 11.0f, 11.0f}, { 0.0f, 0.8f, 0.0f}, 0.0f, 0.0f, 0.0f};
					materials[6] = {{                    }, { 0.999f, 0.999f, 0.999f}, 0.0f, 0.99f, 0.0f};
					materials[7] = {{                    }, { 0.999f, 0.999f, 0.999f}, 1.4f, 0.0f, 0.0f};
					materials[8] = {{                    }, { 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f};
					
					Sphere* s = spheres;
					world->sphereCount = sphereCount;
					s[0] = {{ -100.5f,    0.0f,   10.0f}, 100.0f, 0};
					s[1] = {{    0.0f,  -99.5f,   10.0f}, 100.0f, 1};
					s[2] = {{  100.5f,    0.0f,   10.0f}, 100.0f, 2};
					s[3] = {{    0.0f,  101.6f,   10.0f}, 100.0f, 3};
					s[4] = {{    0.0f,    0.0f,  -92.6f}, 100.0f, 4};
					s[5] = {{    0.0f,    2.6f,   8.0f},   1.0f, 5};
					s[6] = {{   -0.21f,   0.7f,    7.8f},  0.211f, 6};
					s[7] = {{    0.21f,   0.7f,    8.2f},  0.211f, 7};
					s[8] = {{    0.0f,    0.0f,   110.0f},  100.0f, 8};
					
					createWork(work, &graphBuffer, world, workCount, workExtraX, workExtraY, workWidth, workHeight, sampleCount);
					
					for (int i = 0; i < WORKERCOUNT; i++) {
						threads[i] = CreateThread(
							NULL, 0, ThreadFunction, &work[i], CREATE_SUSPENDED, NULL
						);
						work[i].started = 1;
					}
					
					for (int i = 0; i < WORKERCOUNT; i++) {
						ResumeThread(threads[i]);
					}
					
					sampleCount *= 2;
					startThreads = 0;
					finishedTracing = 0;
					beginClock = clock();
				}
				
				static u32 lastWorkThreadIndex = -1;
				if(!finishedTracing) {
					for(int i = 0; i < WORKERCOUNT; i++) {
						if(threads[i] != 0) {
							DWORD result = WaitForSingleObject( threads[i], 0);
							//printf("Thread %u: DONE? %u\n", i, result == WAIT_OBJECT_0);
							if (result == WAIT_OBJECT_0) {
								CloseHandle(threads[i]);
								threads[i] = 0;
								if(lastWorkThreadIndex == i) {
									lastWorkThreadIndex = -1;
									finishedTracing = 1;
									clock_t endClock = clock() - beginClock;
									u32 milliseconds = endClock % 1000;
									u32 seconds = endClock / 1000;
									u32 minutes = seconds / 60;
									if(minutes > 0) {
										printf("\rRaycasting finished in %u:%u min", minutes, seconds % 60);
									} else if(seconds > 30) {
										printf("\rRaycasting finished in %us:%ums", seconds, milliseconds % 1000);
									} else {
										printf("\rRaycasting finished in %ums", endClock);
									}
									printf(" (%fms/sample)\n",
									(double)endClock/(sampleCount*graphBuffer.width*graphBuffer.height));
								}
								break;
							}
						} else {
							u32 fullWork = workCount + workExtraX + workExtraY + workExtraE;
							for(u32 j = 0; j < fullWork; j++) {
								if(work[j].started == 0 && j == fullWork - 1) {
									u32 percent = (u32)(((f32)j / fullWork)*100);
									threads[i] = CreateThread(
										NULL, 0, ThreadFunction, &work[j], 0, NULL
									);
									work[j].started = 1;
									printf("\rRaycasting   %u%%...", percent);
									lastWorkThreadIndex = i;
									break;
								}
								if(work[j].started == 0) {
									u32 percent = (u32)(((f32)j / fullWork)*100);
									threads[i] = CreateThread(
										NULL, 0, ThreadFunction, &work[j], 0, NULL
									);
									work[j].started = 1;
									printf("\rRaycasting   %u%%...", percent);
									break;
								}
							}
						}
					}
				}
				
				if(!threadsPaused && pauseThreads) {
					for(u32 i = 0; i < WORKERCOUNT; i++) {
						if(threads[i] != 0) {
							SuspendThread(threads[i]);
						}
					}
					threadsPaused = 1;
				}
				if(threadsPaused && !pauseThreads) {
					for(u32 i = 0; i < WORKERCOUNT; i++) {
						if(threads[i] != 0) {
							ResumeThread(threads[i]);
						}
					}
					threadsPaused = 0;
				}
				
				glViewport(0,0, windowWidth, windowHeight);
				glClearColor(1, 1, 1, 1);
				glClear(GL_COLOR_BUFFER_BIT);
				
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				
				glMatrixMode(GL_PROJECTION);
				GLfloat projection[] = 
				{
					2.0f/windowWidth, 0.0f, 0.0f, 0.0f,
					0.0f, 2.0f/windowHeight, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					-1.0f, -1.0f, 0.0f, 1.0f,
				};
				glLoadMatrixf(projection);
				
				glBindTexture(GL_TEXTURE_2D, 1);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graphBuffer.width, graphBuffer.height, 
					0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)graphBuffer.memory);
				
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 1);
				glBegin(GL_TRIANGLES);
					glTexCoord2f(0.0f, 0.0f);
					glVertex2f(0, 0);
					
					glTexCoord2f(1.0f, 0.0f);
					glVertex2f(windowWidth, 0);
					
					glTexCoord2f(1.0f, 1.0f);
					glVertex2f(windowWidth, windowHeight);
					
					glTexCoord2f(0.0f, 0.0f);
					glVertex2f(0, 0);
					
					glTexCoord2f(1.0f, 1.0f);
					glVertex2f(windowWidth, windowHeight);
					
					glTexCoord2f(0.0f, 1.0f);
					glVertex2f(0, windowHeight);
				glEnd();
				glDisable(GL_TEXTURE_2D);
				
				SwapBuffers(deviceContext);
				Sleep(33.33333f);
			}
		}
	}

	//VirtualFree((void*)memory, 0, MEM_RELEASE);
	return(0);
}

LRESULT CALLBACK WindowProc(HWND window, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// For all messages:
	//		@see https://docs.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues
	switch(uMsg) {
		case WM_DESTROY: 
		case WM_CLOSE:
		case WM_QUIT: {
			isRunning = false;
			PostQuitMessage(0);
			return(0);
		}
		case WM_SIZE: {
			windowWidth = GET_X_LPARAM(lParam);
			windowHeight = GET_Y_LPARAM(lParam);
#if DEBUG
			printf("WSIZE(%u, %u)\n", windowWidth, windowHeight);
#endif
			return(0);
		}
		case WM_KEYUP: {
			if(wParam == 0x52) {
				startThreads = 1;
			} else if(wParam == 0x53)
			{
				writeBufferAsBitmap(&graphBuffer, "output\\test.bmp");
			} else if(wParam == 0x50) {
				pauseThreads = !pauseThreads;
			}
			break;
		}
		case WM_MOUSEMOVE: {
			switch(wParam) {
				case MK_CONTROL: { // ctrl down
					puts("CTRL");
					break;
				} 
				case MK_LBUTTON: { // left mouse
					puts("LBUTTON");
					break;
				} 
				case MK_MBUTTON: { // middle mouse
					puts("MBUTTON");
					break;
				} 
				case MK_RBUTTON: { // right mouse
					puts("RBUTTON");
					break;
				} 
				case MK_SHIFT: { // shift down
					puts("SHIFT");
					break;
				} 
				case MK_XBUTTON1: { // first extra mouse
					puts("XBUTTON1");
					break;
				} 
				case MK_XBUTTON2: { // second extra mouse
					puts("XBUTTON2");
					break;
				} 
			}
			mouseX = GET_X_LPARAM(lParam);
			mouseY = GET_Y_LPARAM(lParam);
			
#if 0
			printf("MPos(%u, %u)\n", mouseX, mouseY);
#endif
		}
		case WM_ERASEBKGND: {
			return(1);
		}
	}
	return(DefWindowProc(window, uMsg, wParam, lParam));
}

/*
GL_EXT_blend_minmax GL_EXT_blend_subtract GL_EXT_blend_color GL_EXT_abgr GL_EXT_texture3D GL_EXT_clip_volume_hint GL_EXT_compiled_vertex_array GL_SGIS_texture_edge_clamp GL_SGIS_generate_mipmap GL_EXT_draw_range_elements GL_SGIS_texture_lod GL_EXT_rescale_normal GL_EXT_packed_pixels GL_EXT_texture_edge_clamp GL_EXT_separate_specular_color GL_ARB_multitexture GL_ARB_map_buffer_alignment GL_ARB_conservative_depth GL_EXT_texture_env_combine GL_EXT_bgra GL_EXT_blend_func_separate GL_EXT_secondary_color GL_EXT_fog_coord GL_EXT_texture_env_add GL_ARB_texture_cube_map GL_ARB_transpose_matrix GL_ARB_internalformat_query GL_ARB_internalformat_query2 GL_ARB_texture_env_add GL_IBM_texture_mirrored_repeat GL_EXT_multi_draw_arrays GL_SUN_multi_draw_arrays GL_NV_blend_square GL_ARB_texture_compression GL_3DFX_texture_compression_FXT1 GL_EXT_texture_filter_anisotropic GL_ARB_texture_border_clamp GL_ARB_point_parameters GL_ARB_texture_env_combine GL_ARB_texture_env_dot3 GL_ARB_texture_env_crossbar GL_EXT_texture_compression_s3tc GL_ARB_shadow GL_ARB_window_pos GL_EXT_shadow_funcs GL_EXT_stencil_wrap GL_ARB_vertex_program GL_EXT_texture_rectangle GL_ARB_fragment_program GL_EXT_stencil_two_side GL_ATI_separate_stencil GL_ARB_vertex_buffer_object GL_EXT_texture_lod_bias GL_ARB_occlusion_query GL_ARB_fragment_shader GL_ARB_shader_objects GL_ARB_shading_language_100 GL_ARB_texture_non_power_of_two GL_ARB_vertex_shader GL_NV_texgen_reflection GL_ARB_point_sprite GL_ARB_fragment_program_shadow GL_EXT_blend_equation_separate GL_ARB_depth_texture GL_ARB_texture_rectangle GL_ARB_draw_buffers GL_ARB_color_buffer_float GL_ARB_half_float_pixel GL_ARB_texture_float GL_ARB_pixel_buffer_object GL_EXT_framebuffer_object GL_ARB_draw_instanced GL_ARB_half_float_vertex GL_ARB_occlusion_query2 GL_EXT_draw_buffers2 GL_WIN_swap_hint GL_EXT_texture_sRGB GL_ARB_multisample GL_EXT_packed_float GL_EXT_texture_shared_exponent GL_ARB_texture_rg GL_ARB_texture_compression_rgtc GL_NV_conditional_render GL_ARB_texture_swizzle GL_EXT_texture_swizzle GL_ARB_texture_gather GL_ARB_sync GL_ARB_framebuffer_sRGB GL_EXT_packed_depth_stencil GL_ARB_depth_buffer_float GL_EXT_transform_feedback GL_ARB_transform_feedback2 GL_ARB_draw_indirect GL_EXT_framebuffer_blit GL_EXT_framebuffer_multisample GL_ARB_framebuffer_object GL_ARB_framebuffer_no_attachments GL_EXT_texture_array GL_EXT_texture_integer GL_ARB_map_buffer_range GL_ARB_texture_buffer_range GL_EXT_texture_buffer GL_EXT_texture_snorm GL_ARB_blend_func_extended GL_INTEL_performance_queries GL_INTEL_performance_query GL_ARB_copy_buffer GL_ARB_sampler_objects GL_NV_primitive_restart GL_ARB_seamless_cube_map GL_ARB_uniform_buffer_object GL_ARB_depth_clamp GL_ARB_vertex_array_bgra GL_ARB_shader_bit_encoding GL_ARB_draw_buffers_blend GL_ARB_geometry_shader4 GL_EXT_geometry_shader4 GL_ARB_texture_query_lod GL_ARB_explicit_attrib_location GL_ARB_draw_elements_base_vertex GL_ARB_instanced_arrays GL_ARB_base_instance GL_ARB_fragment_coord_conventions GL_EXT_gpu_program_parameters GL_ARB_texture_buffer_object_rgb32 GL_ARB_compatibility GL_ARB_texture_rgb10_a2ui GL_ARB_texture_multisample GL_ARB_vertex_type_2_10_10_10_rev GL_ARB_timer_query GL_ARB_tessellation_shader GL_ARB_vertex_array_object GL_ARB_provoking_vertex GL_ARB_sample_shading GL_ARB_texture_cube_map_array GL_EXT_gpu_shader4 GL_ARB_gpu_shader5 GL_ARB_gpu_shader_fp64 GL_ARB_shader_subroutine GL_ARB_transform_feedback3 GL_ARB_get_program_binary GL_ARB_separate_shader_objects GL_ARB_shader_precision GL_ARB_vertex_attrib_64bit GL_ARB_viewport_array GL_ARB_transform_feedback_instanced GL_ARB_compressed_texture_pixel_storage GL_ARB_shader_atomic_counters GL_ARB_shading_language_packing GL_ARB_shading_language_420pack GL_ARB_texture_storage GL_EXT_texture_storage GL_ARB_vertex_attrib_binding GL_ARB_multi_draw_indirect GL_ARB_program_interface_query GL_ARB_texture_storage_multisample GL_ARB_buffer_storage GL_ARB_debug_output GL_KHR_debug GL_ARB_arrays_of_arrays GL_INTEL_map_texture GL_ARB_texture_compression_bptc GL_ARB_ES2_compatibility GL_ARB_ES3_compatibility GL_ARB_robustness GL_EXT_texture_sRGB_decode GL_KHR_blend_equation_advanced GL_EXT_shader_integer_mix GL_ARB_stencil_texturing
*/

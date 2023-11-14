/**
 * This is an extension of SDL2 for WebGPU, abstracting away the details of
 * OS-specific operations.
 * 
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * Most of this code comes from the wgpu-native triangle example:
 *   https://github.com/gfx-rs/wgpu-native/blob/master/examples/triangle/main.c
 * 
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel and the wgpu-native authors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "sdl2webgpu.h"

#include <webgpu/webgpu.h>

#if defined(SDL_VIDEO_DRIVER_COCOA)
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

WGPUSurface SDL_GetWGPUSurface(WGPUInstance instance, SDL_Window* window) {
    SDL_SysWMinfo windowWMInfo;
    SDL_VERSION(&windowWMInfo.version);
    SDL_GetWindowWMInfo(window, &windowWMInfo);

#if defined(SDL_VIDEO_DRIVER_COCOA)
    {
        id metal_layer = NULL;
        NSWindow* ns_window = windowWMInfo.info.cocoa.window;
        [ns_window.contentView setWantsLayer : YES] ;
        metal_layer = [CAMetalLayer layer];
        [ns_window.contentView setLayer : metal_layer] ;
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromMetalLayer) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromMetalLayer,
                },
                .layer = metal_layer,
            },
        });
    }
#elif defined(SDL_VIDEO_DRIVER_X11)
    {
        Display* x11_display = windowWMInfo.info.x11.display;
        Window x11_window = windowWMInfo.info.x11.window;
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromXlibWindow) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
                },
                .display = x11_display,
                .window = x11_window,
            },
        });
    }
#elif defined(SDL_VIDEO_DRIVER_WAYLAND)
    {
        struct wl_display* wayland_display = windowWMInfo.info.wl.display;
        struct wl_surface* wayland_surface = windowWMInfo.info.wl.display;
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromWaylandSurface) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType =
                            WGPUSType_SurfaceDescriptorFromWaylandSurface,
                        },
                        .display = wayland_display,
                        .surface = wayland_surface,
                },
        });
  }
#elif defined(SDL_VIDEO_DRIVER_WINDOWS)
    {
        HWND hwnd = windowWMInfo.info.win.window;
        HINSTANCE hinstance = GetModuleHandle(NULL);
        return wgpuInstanceCreateSurface(
            instance,
            &(WGPUSurfaceDescriptor){
            .label = NULL,
                .nextInChain =
                (const WGPUChainedStruct*)&(
                    WGPUSurfaceDescriptorFromWindowsHWND) {
                .chain =
                    (WGPUChainedStruct){
                        .next = NULL,
                        .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
            },
            .hinstance = hinstance,
            .hwnd = hwnd,
        },
    });
  }
#else
    // TODO: See SDL_syswm.h for other possible enum values!
#error "Unsupported WGPU_TARGET"
#endif
}


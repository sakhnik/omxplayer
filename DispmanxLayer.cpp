//
// The MIT License (MIT)
//
// Copyright (c) 2020 Michael Walsh
//
// Based on pngview by Andrew Duncan
// https://github.com/AndrewFromMelbourne/raspidmx/tree/master/pngview
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <assert.h>
#include <bcm_host.h>

#include "DispmanxLayer.h"

typedef struct {
	struct IMAGE_T {
		int32_t width;
		int32_t height;
		int32_t pitch;
		uint32_t size;
	} image;
	VC_RECT_T bmpRect;
	VC_RECT_T srcRect;
	VC_RECT_T dstRect;
	int32_t layer;
	DISPMANX_RESOURCE_HANDLE_T resource;
	DISPMANX_ELEMENT_HANDLE_T element;
} IMAGE_LAYER_T;

static DISPMANX_DISPLAY_HANDLE_T m_display;
static DISPMANX_UPDATE_HANDLE_T m_update;
static IMAGE_LAYER_T image_layer;

static bool element_is_hidden = false;

#define ELEMENT_CHANGE_LAYER	(1<<0)

void openDisplay(int display_num, int &screen_width, int &screen_height)
{
	bcm_host_init();

	// Open display
	m_display = vc_dispmanx_display_open(display_num);
	assert(m_display != 0);

	// Get screen info
	DISPMANX_MODEINFO_T screen_info;
	int result = vc_dispmanx_display_get_info(m_display, &screen_info);
	assert(result == 0);

	screen_width = screen_info.width;
	screen_height = screen_info.height;
}

void createImageLayer(int32_t layer, int32_t margin_left,
	int32_t margin_top, int32_t width, int32_t height)
{
	// Init image note dimensions should be divisible by 16
	assert(width % 16 == 0);
	assert(height % 16 == 0);

	uint32_t vc_image_ptr;
	image_layer.layer = layer;

	// Init image
	image_layer.image.width = width;
	image_layer.image.height = height;
	image_layer.image.pitch = width * 4;
	image_layer.image.size = image_layer.image.pitch * height;

	image_layer.resource = vc_dispmanx_resource_create(
		VC_IMAGE_RGBA32,
		width | (image_layer.image.pitch << 16),
		height | (image_layer.image.height << 16),
		&vc_image_ptr);

	assert(image_layer.resource != 0);

	// set bmpRect as being whole image
	vc_dispmanx_rect_set(&(image_layer.bmpRect), 0, 0,
		image_layer.image.width, image_layer.image.height);

	// Position currently empty image on screen
	m_update = vc_dispmanx_update_start(0);
	assert(m_update != 0);

	vc_dispmanx_rect_set(&(image_layer.srcRect), 0 << 16, 0 << 16,
		image_layer.image.width << 16, image_layer.image.height << 16);

	vc_dispmanx_rect_set(&(image_layer.dstRect), margin_left, margin_top,
		image_layer.image.width, image_layer.image.height);

	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };

	image_layer.element = vc_dispmanx_element_add(m_update, m_display, image_layer.layer, 
		&(image_layer.dstRect), image_layer.resource, &(image_layer.srcRect),
		DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);

	assert(image_layer.element != 0);

	int result = vc_dispmanx_update_submit_sync(m_update);
	assert(result == 0);
}

void changeImageLayer(int new_layer)
{
	m_update = vc_dispmanx_update_start(0);
	assert(m_update != 0);

	// change layer to new_layer
	int ret = vc_dispmanx_element_change_attributes(m_update, image_layer.element,
		ELEMENT_CHANGE_LAYER, new_layer, 255, NULL, NULL, 0, DISPMANX_NO_ROTATE);

	ret = vc_dispmanx_update_submit_sync( m_update );
	assert( ret == 0 );
}

void hideElement()
{
	if(element_is_hidden) return;
	changeImageLayer(-30);
	element_is_hidden = true;
}

void showElement()
{
	if(!element_is_hidden) return;
	changeImageLayer(image_layer.layer);
	element_is_hidden = false;
}

// copy image data to screen and make the element visible
void setImageData(void *image_data)
{
	int result = vc_dispmanx_resource_write_data(image_layer.resource,
		VC_IMAGE_RGBA32, image_layer.image.pitch, image_data, &(image_layer.bmpRect));

	assert(result == 0);

	result = vc_dispmanx_element_change_source(m_update, image_layer.element,
		image_layer.resource);

	assert(result == 0);

	showElement();
}

void removeImageLayer()
{
	int result = 0;

	DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
	assert(update != 0);

	result = vc_dispmanx_element_remove(update, image_layer.element);
	assert(result == 0);

	result = vc_dispmanx_update_submit_sync(update);
	assert(result == 0);

	result = vc_dispmanx_resource_delete(image_layer.resource);
	assert(result == 0);

	result = vc_dispmanx_display_close(m_display);
	assert(result == 0);
}

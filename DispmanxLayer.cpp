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

#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

DISPMANX_DISPLAY_HANDLE_T DispmanxLayer::m_display;

void DispmanxLayer::openDisplay(int display_num, int &screen_width, int &screen_height)
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

void DispmanxLayer::closeDisplay()
{
	int result = vc_dispmanx_display_close(m_display);
	assert(result == 0);
}

DispmanxLayer::DispmanxLayer(int32_t layer, int32_t margin_left, int32_t margin_top, int pitch,
		int32_t dst_image_width, int32_t dst_image_height,
		int32_t src_image_width, int32_t src_image_height)
{
	// palette
	assert(pitch == 2 || pitch == 4);
	VC_IMAGE_TYPE_T palette = pitch == 4 ? VC_IMAGE_ARGB8888 : VC_IMAGE_RGBA16;

	if(src_image_width == -1) src_image_width = dst_image_width;
	if(src_image_height == -1) src_image_height = dst_image_height;

	// Destination image dimensions should be divisible by 16
	assert(dst_image_width % 16 == 0);
	assert(dst_image_height % 16 == 0);

	// set image rectangles
	VC_RECT_T srcRect;
	VC_RECT_T dstRect;
	vc_dispmanx_rect_set(&(m_bmpRect), 0, 0, src_image_width, src_image_height);
	vc_dispmanx_rect_set(&(srcRect), 0 << 16, 0 << 16, src_image_width << 16, src_image_height << 16);
	vc_dispmanx_rect_set(&(dstRect), margin_left, margin_top, dst_image_width, dst_image_height);

	// Image vars
	m_image_pitch = src_image_width * pitch;
	m_layer = layer;

	// create image resource
	uint32_t vc_image_ptr;
	m_resource = vc_dispmanx_resource_create(
		palette,
		src_image_width | (m_image_pitch << 16),
		src_image_height | (src_image_height << 16),
		&vc_image_ptr);
	assert(m_resource != 0);

	// Position currently empty image on screen
	m_update = vc_dispmanx_update_start(0);
	assert(m_update != 0);

	VC_DISPMANX_ALPHA_T alpha = { DISPMANX_FLAGS_ALPHA_FROM_SOURCE, 255, 0 };

	m_element = vc_dispmanx_element_add(m_update, m_display, -30,
		&(dstRect), m_resource, &(srcRect),
		DISPMANX_PROTECTION_NONE, &alpha, NULL, DISPMANX_NO_ROTATE);

	assert(m_element != 0);

	int result = vc_dispmanx_update_submit_sync(m_update);
	assert(result == 0);
}

void DispmanxLayer::changeImageLayer(int new_layer)
{
	m_update = vc_dispmanx_update_start(0);
	assert(m_update != 0);

	// change layer to new_layer
	int ret = vc_dispmanx_element_change_attributes(m_update, m_element,
		ELEMENT_CHANGE_LAYER, new_layer, 255, NULL, NULL, 0, DISPMANX_NO_ROTATE);
	assert( ret == 0 );

	ret = vc_dispmanx_update_submit_sync( m_update );
	assert( ret == 0 );
}

void DispmanxLayer::hideElement()
{
	if(m_element_is_hidden) return;
	changeImageLayer(-30);
	m_element_is_hidden = true;
}

void DispmanxLayer::showElement()
{
	if(!m_element_is_hidden) return;
	changeImageLayer(m_layer);
	m_element_is_hidden = false;
}

// copy image data to screen and make the element visible
void DispmanxLayer::setImageData(void *image_data)
{
	// the palette param is ignored
	int result = vc_dispmanx_resource_write_data(m_resource,
		VC_IMAGE_MIN, m_image_pitch, image_data, &(m_bmpRect));

	assert(result == 0);

	result = vc_dispmanx_element_change_source(m_update, m_element, m_resource);

	assert(result == 0);

	showElement();
}

DispmanxLayer::~DispmanxLayer()
{
	int result = 0;

	m_update = vc_dispmanx_update_start(0);
	assert(m_update != 0);

	result = vc_dispmanx_element_remove(m_update, m_element);
	assert(result == 0);

	result = vc_dispmanx_update_submit_sync(m_update);
	assert(result == 0);

	result = vc_dispmanx_resource_delete(m_resource);
	assert(result == 0);
}

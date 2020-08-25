#pragma once
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

#include <bcm_host.h>

class DispmanxLayer
{
public:
	DispmanxLayer(int32_t layer, int32_t margin_left, int32_t margin_top, int pitch,
		int32_t src_width, int32_t src_height,
		int32_t dst_width = -1, int32_t dst_height = -1);
	~DispmanxLayer();

	void hideElement();
	void setImageData(void *image_data);

	static void openDisplay(int display_num, int &screen_width, int &screen_height);
	static void closeDisplay();

private:
	void changeImageLayer(int new_layer);
	void showElement();

	VC_RECT_T m_bmpRect;
	int32_t m_layer;
	int32_t m_image_pitch;
	DISPMANX_RESOURCE_HANDLE_T m_resource;
	DISPMANX_ELEMENT_HANDLE_T m_element;
	DISPMANX_UPDATE_HANDLE_T m_update;

	bool m_element_is_hidden = true;
	static DISPMANX_DISPLAY_HANDLE_T m_display;
};

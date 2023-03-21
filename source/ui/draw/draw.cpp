#include "headers.hpp"
#include "ui/colors.hpp"

namespace Draw_ {
	double draw_frametime[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
	C2D_Font system_fonts[4];
	C3D_RenderTarget* screen[2];
	C2D_SpriteSheet sheet_texture[128];
	C2D_Image wifi_icon_image[9];
	C2D_Image battery_level_icon_image[21];
	C2D_Image battery_charge_icon_image[1];
	C2D_Image eco_image[2];
	u32 draw_japanese_kanji[3001];
	int japanese_kanji_num;
	u32 draw_simple_chinese[6301];
	int simple_chinese_num;
	TickCounter draw_frame_time_timer;
	
	std::string samples_str[8] = {"\u0000", "\u000A", "\u4DFF", "\uA000", "\u312F", "\u3190", "\uABFF", "\uD7B0"};
	u32 samples[8];
	
	static inline u32 str2u32(const std::string &str) {
		u32 res = 0;
		for (char c : str) res = res * 0x100 + (u8) c;
		return res;
	}
};
using namespace Draw_;

double Draw_query_frametime(void)
{
	return draw_frametime[19];
}

double Draw_query_fps(void)
{
	double cache = 0;
	for(int i = 0; i < 20; i++)
		cache += draw_frametime[i];

	return 1000.0 / (cache / 20);
}

extern "C" void memcpy_asm_4b(u8*, u8*);

int Draw_convert_to_pos(int height, int width, int img_height, int img_width, int pixel_size)
{
	int pos = img_width * height;
	if(pos == 0)
		pos = img_width;

	pos -= (img_width - width) - img_width;
	return pos * pixel_size;
}

Result_with_string Draw_set_texture_data(Image_data* c2d_image, u8* buf, int pic_width, int pic_height, int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format)
{
	return Draw_set_texture_data(c2d_image, buf, pic_width, pic_height, 0, 0, tex_size_x, tex_size_y, color_format);
}

Result_with_string Draw_set_texture_data(Image_data* c2d_image, u8* buf, int pic_width, int pic_height, int parse_start_width, int parse_start_height, int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format)
{
	int x_max = 0;
	int y_max = 0;
	int increase_list_x[tex_size_x + 8]; //= { 4, 12, 4, 44, }
	int increase_list_y[tex_size_y + 8]; //= { 2, 6, 2, 22, 2, 6, 2, tex_size_x * 8 - 42, };
	int count[2] = { 0, 0, };
	int c3d_pos = 0;
	int c3d_offset = 0;
	int pixel_size = 0;
	Result_with_string result;

	if(color_format == GPU_RGB8)
		pixel_size = 3;
	else if(color_format == GPU_RGB565)
		pixel_size = 2;
	else
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}

	for(int i = 0; i <= tex_size_x; i+=4)
	{
		increase_list_x[i] = 4 * pixel_size;
		increase_list_x[i + 1] = 12 * pixel_size;
		increase_list_x[i + 2] = 4 * pixel_size;
		increase_list_x[i + 3] = 44 * pixel_size;
	}
	for(int i = 0; i <= tex_size_y; i+=8)
	{
		increase_list_y[i] = 2 * pixel_size;
		increase_list_y[i + 1] = 6 * pixel_size;
		increase_list_y[i + 2] = 2 * pixel_size;
		increase_list_y[i + 3] = 22 * pixel_size;
		increase_list_y[i + 4] = 2 * pixel_size;
		increase_list_y[i + 5] = 6 * pixel_size;
		increase_list_y[i + 6] = 2 * pixel_size;
		increase_list_y[i + 7] = (tex_size_x * 8 - 42) * pixel_size;
	}

	if (parse_start_width > pic_width || parse_start_height > pic_height)
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}
	
	y_max = pic_height - (u32)parse_start_height;
	x_max = pic_width - (u32)parse_start_width;
	if (tex_size_y < y_max)
		y_max = tex_size_y;
	if (tex_size_x < x_max)
		x_max = tex_size_x;

	c2d_image->subtex->width = (u16)x_max;
	c2d_image->subtex->height = (u16)y_max;
	c2d_image->subtex->left = 0.0;
	c2d_image->subtex->top = 1.0;
	c2d_image->subtex->right = x_max / (float)tex_size_x;
	c2d_image->subtex->bottom = 1.0 - y_max / (float)tex_size_y;
	c2d_image->c2d.subtex = c2d_image->subtex;

	if(pixel_size == 2)
	{
		for(int k = 0; k < y_max; k++)
		{
			for(int i = 0; i < x_max; i += 2)
			{
				memcpy_asm_4b(&(((u8*)c2d_image->c2d.tex->data)[c3d_pos + c3d_offset]), &(((u8*)buf)[Draw_convert_to_pos(k + parse_start_height, i + parse_start_width, pic_height, pic_width, pixel_size)]));
				c3d_pos += increase_list_x[count[0]];
				count[0]++;
			}
			count[0] = 0;
			c3d_pos = 0;
			c3d_offset += increase_list_y[count[1]];
			count[1]++;
		}
	}
	else if(pixel_size == 3)
	{
		for(int k = 0; k < y_max; k++)
		{
			for(int i = 0; i < x_max; i += 2)
			{
				memcpy_asm_4b(&(((u8*)c2d_image->c2d.tex->data)[c3d_pos + c3d_offset]), &(((u8*)buf)[Draw_convert_to_pos(k + parse_start_height, i + parse_start_width, pic_height, pic_width, pixel_size)]));
				memcpy(&(((u8*)c2d_image->c2d.tex->data)[c3d_pos + c3d_offset + 4]), &(((u8*)buf)[Draw_convert_to_pos(k + parse_start_height, i + parse_start_width, pic_height, pic_width, pixel_size) + 4]), 2);
				c3d_pos += increase_list_x[count[0]];
				count[0]++;
			}
			count[0] = 0;
			c3d_pos = 0;
			c3d_offset += increase_list_y[count[1]];
			count[1]++;
		}
	}

	C3D_TexFlush(c2d_image->c2d.tex);

	return result;
}

void Draw_c2d_image_set_filter(Image_data* c2d_image, bool filter)
{
	if(filter)
		C3D_TexSetFilter(c2d_image->c2d.tex, GPU_LINEAR, GPU_LINEAR);
	else
		C3D_TexSetFilter(c2d_image->c2d.tex, GPU_NEAREST, GPU_NEAREST);
}

Result_with_string Draw_c2d_image_init(Image_data* c2d_image, int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format)
{
	Result_with_string result;

	c2d_image->subtex = (Tex3DS_SubTexture*)linearAlloc_concurrent(sizeof(Tex3DS_SubTexture*));
	c2d_image->c2d.tex = (C3D_Tex*)linearAlloc_concurrent(sizeof(C3D_Tex*));
	if(c2d_image->subtex == NULL || c2d_image->c2d.tex == NULL)
	{
		linearFree_concurrent(c2d_image->subtex);
		linearFree_concurrent(c2d_image->c2d.tex);
		c2d_image->subtex = NULL;
		c2d_image->c2d.tex = NULL;
		result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
		result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	}
	c2d_image->c2d.subtex = c2d_image->subtex;

	if (!C3D_TexInit(c2d_image->c2d.tex, (u16)tex_size_x, (u16)tex_size_y, color_format))
	{
		result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
		result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
		return result;
	}

	C3D_TexSetFilter(c2d_image->c2d.tex, GPU_LINEAR, GPU_LINEAR);
	c2d_image->c2d.tex->border = 0xFFFFFF;
	C3D_TexSetWrap(c2d_image->c2d.tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);


	return result;
}

void Draw_c2d_image_free(Image_data c2d_image)
{
	linearFree_concurrent(c2d_image.c2d.tex->data);
	linearFree_concurrent(c2d_image.c2d.tex);
	linearFree_concurrent(c2d_image.subtex);
	c2d_image.c2d.tex->data = NULL;
	c2d_image.c2d.tex = NULL;
	c2d_image.c2d.subtex = NULL;
	c2d_image.subtex = NULL;
}

void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888)
{
	bool reverse = false;
	bool found = false;
	bool font_loaded[2] = { Extfont_is_sysfont_loaded(0), Extfont_is_sysfont_loaded(1), };//JPN, CHN
	float width = 0, height = 0, original_x, y_offset;
	static int font_num_list[1024];
	static u32 draw_part_text[1024];
	static C2D_Text c2d_text;
	static C2D_TextBuf c2d_buf;
	original_x = x;
	if (!c2d_buf) c2d_buf = C2D_TextBufNew(4096);

	int characters = Extfont_parse_utf8_str_to_u32(text.c_str(), draw_part_text, 1024);
	Extfont_sort_rtl(draw_part_text, characters);

	for (int i = 0; i < characters; i++) {
		u32 cur_char = draw_part_text[i];
		
		if (cur_char == samples[0]) { // NULL
			font_num_list[i] = -2;
			break;
		}
		if (cur_char == samples[1]) { // linebreak
			font_num_list[i] = -1;
			continue;
		}
		
		if (cur_char > samples[2] && cur_char < samples[3]) {
			bool found = false;
			if (font_loaded[0]) { // search Japanese character list for the character
				// binary search
				// std::lower_bound(start, end, target_value) assumes [start, end) is non-decreasing
				// and returns the pointer to the first element that is equal to or greater than target_value(end if none)
				auto ptr = std::lower_bound(draw_japanese_kanji, draw_japanese_kanji + japanese_kanji_num, cur_char);
				if (ptr < draw_japanese_kanji + japanese_kanji_num && *ptr == cur_char) {
					found = true;
					font_num_list[i] = 0; // JPN
				}
			}
			if (!found && font_loaded[1]) { // search simple Chinese character list
				auto ptr = std::lower_bound(draw_simple_chinese, draw_simple_chinese + simple_chinese_num, cur_char);
				if (ptr < draw_simple_chinese + simple_chinese_num && *ptr == cur_char) {
					found = true;
					font_num_list[i] = 1; // CHN
				}
			}
			if (!found) font_num_list[i] = 3; // TWN
		} else if ((cur_char > samples[4] && cur_char < samples[5]) || 
				   (cur_char > samples[6] && cur_char < samples[7])) {
			font_num_list[i] = 2; // KOR
		} else font_num_list[i] = 4;
	}
	font_num_list[characters] = -2;
	
	int prev_font_list_num = font_num_list[0];
	int consecutive_start = 0;
	for (int i = 0; i <= characters; i++) { // process until the last NULL to get rid of additional processing of the last consecutive block
		int cur_font_list_num = font_num_list[i];
		if (prev_font_list_num != cur_font_list_num || cur_font_list_num == -1 || cur_font_list_num == -2) { // different font list, linebreak or the end of string
			if (prev_font_list_num == -1) { // linebreak
				y += 20.0 * text_size_y;
				x = original_x;
			} else if (!Extfont_is_extfont_loaded(0) || (prev_font_list_num >= 0 && prev_font_list_num <= 3)) {
				C2D_Font cur_font = Extfont_is_extfont_loaded(0) ? system_fonts[prev_font_list_num] : NULL;
				
				C2D_TextBufClear(c2d_buf);
				if (prev_font_list_num == 1) y_offset = 3 * text_size_y;
				else if (prev_font_list_num == 3) y_offset = 5 * text_size_y;
				else y_offset = 0;
				
				std::string draw_str;
				for (int j = consecutive_start; j < i; j++) {
					u32 cur_char = draw_part_text[j];
					if (cur_char >> 24) draw_str.push_back(cur_char >> 24);
					if (cur_char >> 16) draw_str.push_back(cur_char >> 16 & 0xFF);
					if (cur_char >> 8) draw_str.push_back(cur_char >> 8 & 0xFF);
					draw_str.push_back(cur_char & 0xFF); // assuming cur_char != 0
				}

				C2D_TextFontParse(&c2d_text, cur_font, c2d_buf, draw_str.c_str());
				C2D_TextOptimize(&c2d_text);
				C2D_TextGetDimensions(&c2d_text, text_size_x, text_size_y, &width, &height);
				C2D_DrawText(&c2d_text, C2D_WithColor, x, y + y_offset, 0.0, text_size_x, text_size_y, abgr8888);
				x += width;
			} else if (prev_font_list_num == 4) {
				Extfont_draw_extfonts(draw_part_text + consecutive_start, i - consecutive_start, x, y, text_size_x * 1.56, text_size_y * 1.56, abgr8888, &width);
				x += width;
			}
			
			prev_font_list_num = cur_font_list_num;
			consecutive_start = i;
		}
		if (cur_font_list_num == -2) break;
	}
}

float Draw_get_height(const std::string &text, float text_size_y) {
	int lines = 1 + std::count(text.begin(), text.end(), '\n');
	return lines * 20.0 * text_size_y;
}

float Draw_get_width_one(u32 cur_char, float text_size_x) {
	if (cur_char == samples[0] || cur_char == samples[1]) return 0; // NULL, linebreak
	
	bool font_loaded[2] = { Extfont_is_sysfont_loaded(0), Extfont_is_sysfont_loaded(1), };//JPN, CHN
	
	int font_list_num;
	if (cur_char > samples[2] && cur_char < samples[3]) {
		bool found = false;
		if (font_loaded[0]) { // search Japanese character list for the character
			// binary search
			// std::lower_bound(start, end, target_value) assumes [start, end) is non-decreasing
			// and returns the pointer to the first element that is equal to or greater than target_value(end if none)
			auto ptr = std::lower_bound(draw_japanese_kanji, draw_japanese_kanji + japanese_kanji_num, cur_char);
			if (ptr < draw_japanese_kanji + japanese_kanji_num && *ptr == cur_char) {
				found = true;
				font_list_num = 0; // JPN
			}
		}
		if (!found && font_loaded[1]) { // search simple Chinese character list
			auto ptr = std::lower_bound(draw_simple_chinese, draw_simple_chinese + simple_chinese_num, cur_char);
			if (ptr < draw_simple_chinese + simple_chinese_num && *ptr == cur_char) {
				found = true;
				font_list_num = 1; // CHN
			}
		}
		if (!found) font_list_num = 3; // TWN
	} else if ((cur_char > samples[4] && cur_char < samples[5]) || 
			   (cur_char > samples[6] && cur_char < samples[7])) {
		font_list_num = 2; // KOR
	} else font_list_num = 4;
	
	float res = 0;
	if (!Extfont_is_extfont_loaded(0) || (font_list_num >= 0 && font_list_num <= 3)) {
		C2D_Font cur_font = Extfont_is_extfont_loaded(0) ? system_fonts[font_list_num] : NULL;
		
		u8 buf[5] = { 0 };
		u8 *ptr = buf;
		if (cur_char >> 24) *ptr++ = cur_char >> 24;
		if (cur_char >> 16) *ptr++ = cur_char >> 16 & 0xFF;
		if (cur_char >> 8) *ptr++ = cur_char >> 8 & 0xFF;
		*ptr++ = cur_char & 0xFF;
		
		fontGlyphPos_s glyphData;
		u32 code;
		if (decode_utf8(&code, (u8 *) buf) == -1) code = 0xFFFD;
		C2D_FontCalcGlyphPos(cur_font, &glyphData, C2D_FontGlyphIndexFromCodePoint(cur_font, code), 0, 1.0f, 1.0f);
		res = glyphData.xAdvance * text_size_x;
	} else if (font_list_num == 4) res = Extfont_get_width_one(cur_char, text_size_x * 1.56);
	return res;
}
float Draw_get_width(const std::string &text, float text_size_x) {
	float x = 0, x_max = 0;
	
	std::vector<u32> part_text(text.size());
	int characters = Extfont_parse_utf8_str_to_u32(text.c_str(), &part_text[0], text.size());
	Extfont_sort_rtl(&part_text[0], characters);
	
	for (int i = 0; i < characters; i++) {
		u32 cur_char = part_text[i];
		if (!cur_char) break;
		if (cur_char == (u8) '\n') {
			x = 0;
			continue;
		}
		x += Draw_get_width_one(cur_char, text_size_x);
		
		x_max = std::max(x_max, x);
	}
	
	return x_max;
}

void Draw_x_centered(std::string text, float x0, float x1, float y, float text_size_x, float text_size_y, int abgr8888) {
	float x = x0 + (x1 - x0 - Draw_get_width(text, text_size_x)) / 2;
	Draw(text, x, y, text_size_x, text_size_y, abgr8888);
}
void Draw_y_centered(std::string text, float x, float y0, float y1, float text_size_x, float text_size_y, int abgr8888) {
	float y = y0 + (y1 - y0 - Draw_get_height(text, text_size_x)) / 2;
	Draw(text, x, y, text_size_x, text_size_y, abgr8888);
}
void Draw_xy_centered(std::string text, float x0, float x1, float y0, float y1, float text_size_x, float text_size_y, int abgr8888) {
	float x = x0 + (x1 - x0 - Draw_get_width(text, text_size_x)) / 2;
	float y = y0 + (y1 - y0 - Draw_get_height(text, text_size_y)) / 2;
	Draw(text, x, y, text_size_x, text_size_y, abgr8888);
}
void Draw_right(std::string text, float x1, float y, float text_size_x, float text_size_y, int abgr8888) {
	Draw(text, x1 - Draw_get_width(text, text_size_x), y, text_size_x, text_size_y, abgr8888);
}

Result_with_string Draw_load_texture(std::string file_name, int sheet_map_num, C2D_Image return_image[], int start_num, int num_of_array)
{
	size_t num_of_images;
	bool function_fail = false;
	Result_with_string load_texture_result;

	sheet_texture[sheet_map_num] = C2D_SpriteSheetLoad(file_name.c_str());
	if (sheet_texture[sheet_map_num] == NULL)
	{
		load_texture_result.code = -1;
		load_texture_result.string = "[Error] Couldn't load texture file : " + file_name + " ";
		function_fail = true;
	}

	if (!function_fail)
	{
		num_of_images = C2D_SpriteSheetCount(sheet_texture[sheet_map_num]);
		if ((int)num_of_images < num_of_array)
		{
			load_texture_result.code = -2;
			load_texture_result.string = "[Error] num of arry " + std::to_string(num_of_array) + " is bigger than spritesheet has num of image(s) " + std::to_string(num_of_images) + " ";
			function_fail = true;
		}
	}

	if (!function_fail)
	{
		for (int i = 0; i <= (num_of_array - 1); i++)
			return_image[start_num + i] = C2D_SpriteSheetGetImage(sheet_texture[sheet_map_num], i);
	}
	return load_texture_result;
}

void Draw_touch_pos(void)
{
	Hid_info key;
	Util_hid_query_key_state(&key);
	if(key.p_touch || key.h_touch) Draw_texture(var_square_image[0], DEF_DRAW_RED, key.touch_x - 1, key.touch_y - 1, 3, 3);
}

void Draw_top_ui(void)
{
	Draw_texture(var_square_image[0], DEF_DRAW_BLACK, 0.0, 0.0, 400.0, 15.0);
	Draw_texture(wifi_icon_image[var_wifi_signal], DEF_DRAW_NO_COLOR, 360.0, 0.0, 15.0, 15.0);
	Draw_texture(battery_level_icon_image[var_battery_level_raw / 5], DEF_DRAW_NO_COLOR, 315.0, 0.0, 30.0, 15.0);
	Draw_texture(eco_image[var_eco_mode], DEF_DRAW_NO_COLOR, 345.0, 0.0, 15.0, 15.0);
	if (var_battery_charge)
		Draw_texture(battery_charge_icon_image[0], DEF_DRAW_NO_COLOR, 295.0, 0.0, 20.0, 15.0);
	Draw(var_status, 0.0, 0.0, 0.45, 0.45, DEF_DRAW_WHITE);
	Draw(std::to_string(var_battery_level_raw), 322.5, 1.25, 0.4, 0.4, DEF_DRAW_BLACK);
}

void Draw_bot_ui(void)
{
	Draw_texture(var_square_image[0], DEF_DRAW_BLACK, 0.0, 225.0, 320.0, 15.0);
	Draw("▽", 155.0, 220.0, 0.75, 0.75, DEF_DRAW_WHITE);
}

void Draw_texture(C2D_Image image, float x, float y, float x_size, float y_size)
{
	Draw_texture(image, DEF_DRAW_NO_COLOR, x, y, x_size, y_size);
}

void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size)
{
	C2D_ImageTint tint;
	C2D_DrawParams c2d_parameter =
	{
		{
			x,
			y,
			x_size,
			y_size
		},
		{
			0,
			0
		},
		0.0f,
		0.0f
	};

	if (!(image.tex == NULL))
	{
		if(abgr8888 == DEF_DRAW_NO_COLOR)
			C2D_DrawImage(image, &c2d_parameter, NULL);
		else
		{
			C2D_PlainImageTint(&tint, abgr8888, true);
			C2D_DrawImage(image, &c2d_parameter, &tint);
		}
	}
}

void Draw_line(float x_0, float y_0, int abgr8888_0, float x_1, float y_1, int abgr8888_1, float width)
{
	if (y_1 - y_0 < 0.01 && abgr8888_0 == abgr8888_1) { // workaround to prevent the line from disappearing sometimes
		Draw_texture(var_square_image[0], abgr8888_0, x_0, y_0, x_1 - x_0 + 1, y_1 - y_0 + 1);
	} else {
		C2D_DrawRectangle(0, 0, 0, 0, 0, 0x0, 0x0, 0x0, 0x0);
		//magic C2D_DrawLine() won't work without calling C2D_DrawRectangle()
		C2D_DrawLine(x_0, y_0, abgr8888_0, x_1, y_1, abgr8888_1, width, 0);
	}
}

void Draw_debug_info(void)
{
	int color = DEF_DRAW_BLACK;
	Hid_info key;
	Util_hid_query_key_state(&key);

	if (var_night_mode)
		color = DEF_DRAW_WHITE;

	Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLUE, 0.0, 20.0, 70.0, 140.0);
	Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLUE, 0.0, 160.0, 110.0, 60.0);
	Draw("A　 p: " + std::to_string(key.p_a) + " h: " + std::to_string(key.h_a), 0.0, 20.0, 0.4, 0.4, color);
	Draw("B　 p: " + std::to_string(key.p_b) + " h: " + std::to_string(key.h_b), 0.0, 30.0, 0.4, 0.4, color);
	Draw("X　 p: " + std::to_string(key.p_x) + " h: " + std::to_string(key.h_x), 0.0, 40.0, 0.4, 0.4, color);
	Draw("Y　 p: " + std::to_string(key.p_y) + " h: " + std::to_string(key.h_y), 0.0, 50.0, 0.4, 0.4, color);
	Draw("L　 p: " + std::to_string(key.p_l) + " h: " + std::to_string(key.h_l), 0.0, 60.0, 0.4, 0.4, color);
	Draw("R　 p: " + std::to_string(key.p_r) + " h: " + std::to_string(key.h_r), 0.0, 70.0, 0.4, 0.4, color);
	Draw("C↓ p: " + std::to_string(key.p_c_down) + " h: " + std::to_string(key.h_c_down), 0.0, 80.0, 0.4, 0.4, color);
	Draw("C→ p: " + std::to_string(key.p_c_right) + " h: " + std::to_string(key.h_c_right), 0.0, 90.0, 0.4, 0.4, color);
	Draw("C↑ p: " + std::to_string(key.p_c_up) + " h: " + std::to_string(key.h_c_up), 0.0, 100.0, 0.4, 0.4, color);
	Draw("C← p: " + std::to_string(key.p_c_left) + " h: " + std::to_string(key.h_c_left), 0.0, 110.0, 0.4, 0.4, color);
	Draw("D↓ p: " + std::to_string(key.p_d_down) + " h: " + std::to_string(key.h_d_down), 0.0, 120.0, 0.4, 0.4, color);
	Draw("D→ p: " + std::to_string(key.p_d_right) + " h: " + std::to_string(key.h_d_right), 0.0, 130.0, 0.4, 0.4, color);
	Draw("D↑ p: " + std::to_string(key.p_d_up) + " h: " + std::to_string(key.h_d_up), 0.0, 140.0, 0.4, 0.4, color);
	Draw("D← p: " + std::to_string(key.p_d_left) + " h: " + std::to_string(key.h_d_left), 0.0, 150.0, 0.4, 0.4, color);
	Draw("touch x: " + std::to_string(key.touch_x) + ", y: " + std::to_string(key.touch_y), 0.0, 160.0, 0.4, 0.4, color);
	Draw("CPU: " + std::to_string(C3D_GetProcessingTime()).substr(0, 5) + "ms", 0.0, 170.0, 0.4, 0.4, color);
	Draw("GPU: " + std::to_string(C3D_GetDrawingTime()).substr(0, 5) + "ms", 0.0, 180.0, 0.4, 0.4, color);
	Draw("Frametime: " + std::to_string(draw_frametime[19]).substr(0, 6) + "ms", 0.0, 190.0, 0.4, 0.4, color);
	Draw("RAM: " + std::to_string(var_free_ram / 1000.0).substr(0, 5) + " MB", 0.0, 200.0, 0.4, 0.4, color);
	Draw("linear RAM: " + std::to_string(var_free_linear_ram / 1000.0 / 1000.0).substr(0, 5) +" MB", 0.0, 210.0, 0.4, 0.4, color);
}

Result_with_string Draw_load_kanji_samples(void)
{
	u8* fs_buffer = (u8*)malloc(0x8000);
	u32 size_read = 0;
	Result_with_string result;

	memset((void*)fs_buffer, 0x0, 0x8000);
	result = Path("romfs:/gfx/font/sample/kanji.txt").read_file(fs_buffer, 0x8000, size_read);
	if (result.code == 0) japanese_kanji_num = Extfont_parse_utf8_str_to_u32((const char *) fs_buffer, draw_japanese_kanji, 3000);
	
	memset((void*)fs_buffer, 0x0, 0x8000);
	result = Path("romfs:/gfx/font/sample/hanyu_s.txt").read_file(fs_buffer, 0x8000, size_read);
	if (result.code == 0) simple_chinese_num = Extfont_parse_utf8_str_to_u32((const char *) fs_buffer, draw_simple_chinese, 6300);

	free(fs_buffer);
	return result;
}

Result_with_string Draw_init(bool wide)
{
	Result_with_string result;
	gfxInitDefault();
	gfxSetWide(wide);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	screen[0] = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	screen[1] = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	C2D_TargetClear(screen[0], C2D_Color32f(0, 0, 0, 0));
	C2D_TargetClear(screen[1], C2D_Color32f(0, 0, 0, 0));
	osTickCounterStart(&draw_frame_time_timer);

	result = Draw_load_texture("romfs:/gfx/draw/wifi_signal.t3x", 0, wifi_icon_image, 0, 9);
	if(result.code != 0)
		return result;

	result = Draw_load_texture("romfs:/gfx/draw/battery_level.t3x", 1, battery_level_icon_image, 0, 21);
	if(result.code != 0)
		return result;

	result = Draw_load_texture("romfs:/gfx/draw/battery_charge.t3x", 2, battery_charge_icon_image, 0, 1);
	if(result.code != 0)
		return result;

	result = Draw_load_texture("romfs:/gfx/draw/eco.t3x", 3, eco_image, 0, 2);
	if(result.code != 0)
		return result;

	result = Draw_load_texture("romfs:/gfx/draw/square.t3x", 4, var_square_image, 0, 1);
	if(result.code != 0)
		return result;

	result = Draw_load_kanji_samples();
	
	for (int i = 0; i < 8; i++) samples[i] = str2u32(samples_str[i]);

	return result;
}

void Draw_reinit(bool wide)
{
	C2D_Fini();
	C3D_Fini();
	//gfxExit();

	//gfxInitDefault();
	gfxSetWide(wide);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	screen[0] = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	screen[1] = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
}

Result_with_string Draw_load_system_font(int system_font_num)
{
	Result_with_string result;
	if (system_font_num == 0)
		system_fonts[0] = C2D_FontLoadSystem(CFG_REGION_JPN);
	else if (system_font_num == 1)
		system_fonts[1] = C2D_FontLoadSystem(CFG_REGION_CHN);
	else if (system_font_num == 2)
		system_fonts[2] = C2D_FontLoadSystem(CFG_REGION_KOR);
	else if (system_font_num == 3)
		system_fonts[3] = C2D_FontLoadSystem(CFG_REGION_TWN);
	else
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}

	if(system_fonts[system_font_num] == NULL)
	{
		result.code = -1;
		result.string = "[Error] Couldn't load font file : " + std::to_string(system_font_num) + " ";
	}
	return result;
}

void Draw_free_system_font(int system_font_num)
{
	if (system_font_num >= 0 && system_font_num <= 3)
	{
		if (system_fonts[system_font_num] != NULL)
		{
			C2D_FontFree(system_fonts[system_font_num]);
			system_fonts[system_font_num] = NULL;
		}
	}
}

void Draw_free_texture(int sheet_map_num)
{
	if (sheet_texture[sheet_map_num] != NULL)
	{
		C2D_SpriteSheetFree(sheet_texture[sheet_map_num]);
		sheet_texture[sheet_map_num] = NULL;
	}
}

void Draw_exit(void)
{
	for (int i = 0; i < 128; i++)
		Draw_free_texture(i);
	for (int i = 0; i < 4; i++)
		Draw_free_system_font(i);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
}

void Draw_frame_ready(void)
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void Draw_screen_ready(int screen_num, int abgr8888)
{
	if (screen_num >= 0 && screen_num <= 1)
	{
		C2D_TargetClear(screen[screen_num], abgr8888);
		C2D_SceneBegin(screen[screen_num]);
	}
}

void Draw_apply_draw(void)
{
	C3D_FrameEnd(0);
	osTickCounterUpdate(&draw_frame_time_timer);
	draw_frametime[19] = osTickCounterRead(&draw_frame_time_timer);
	for(int i = 0; i < 19; i++)
		draw_frametime[i] = draw_frametime[i + 1];
}

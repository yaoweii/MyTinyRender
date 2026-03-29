#include <iostream>
#include <cstring>
#include "../include/TGAImage.h"
namespace YW {
	// 构造函数：创建指定大小的图像，并用指定颜色填充
	TGAImage::TGAImage(const int w, const int h, const int bpp, TGAColor c) : w(w), h(h), bpp(bpp), data(w* h* bpp, 0) {
		for (int j = 0; j < h; j++)
			for (int i = 0; i < w; i++)
				set(i, j, c);  // 设置每个像素的颜色
	}

	// 从文件读取TGA图像
	bool TGAImage::read_tga_file(const std::string filename) {
		std::ifstream in;
		in.open(filename, std::ios::binary);
		if (!in.is_open()) {
			std::cerr << "无法打开文件 " << filename << "\n";
			return false;
		}
		TGAHeader header;
		in.read(reinterpret_cast<char*>(&header), sizeof(header));  // 读取文件头
		if (!in.good()) {
			std::cerr << "读取文件头时发生错误\n";
			return false;
		}
		w = header.width;
		h = header.height;
		bpp = header.bitsperpixel >> 3;  // 将位数转换为字节数
		if (w <= 0 || h <= 0 || (bpp != GRAYSCALE && bpp != RGB && bpp != RGBA)) {
			std::cerr << "无效的bpp（或宽度/高度）值\n";
			return false;
		}
		size_t nbytes = bpp * w * h;
		data = std::vector<std::uint8_t>(nbytes, 0);

		// 根据数据类型码选择不同的解压方式
		if (3 == header.datatypecode || 2 == header.datatypecode) {  // 未压缩的RGB/灰度图像
			in.read(reinterpret_cast<char*>(data.data()), nbytes);
			if (!in.good()) {
				std::cerr << "读取图像数据时发生错误\n";
				return false;
			}
		}
		else if (10 == header.datatypecode || 11 == header.datatypecode) {  // RLE压缩的图像
			if (!load_rle_data(in)) {
				std::cerr << "读取图像数据时发生错误\n";
				return false;
			}
		}
		else {
			std::cerr << "未知的文件格式 " << (int)header.datatypecode << "\n";
			return false;
		}

		// 根据图像描述符的标志位决定是否需要翻转图像
		if (!(header.imagedescriptor & 0x20))
			flip_vertically();  // 第5位=0表示原点在左下角，需要翻转
		if (header.imagedescriptor & 0x10)
			flip_horizontally(); // 第4位=1表示从右到左存储
		std::cerr << w << "x" << h << "/" << bpp * 8 << "\n";
		return true;
	}

	// 加载RLE（行程编码）压缩的数据
	bool TGAImage::load_rle_data(std::ifstream& in) {
		size_t pixelcount = w * h;
		size_t currentpixel = 0;
		size_t currentbyte = 0;
		TGAColor colorbuffer;
		do {
			std::uint8_t chunkheader = 0;
			chunkheader = in.get();  // 读取数据包头部
			if (!in.good()) {
				std::cerr << "读取数据时发生错误\n";
				return false;
			}
			if (chunkheader < 128) {  // 原始数据包：chunkheader+1个像素直接存储
				chunkheader++;
				for (int i = 0; i < chunkheader; i++) {
					in.read(reinterpret_cast<char*>(colorbuffer.bgra), bpp);
					if (!in.good()) {
						std::cerr << "读取数据时发生错误\n";
						return false;
					}
					for (int t = 0; t < bpp; t++)
						data[currentbyte++] = colorbuffer.bgra[t];
					currentpixel++;
					if (currentpixel > pixelcount) {
						std::cerr << "读取的像素过多\n";
						return false;
					}
				}
			}
			else {  // 游程编码包：chunkheader-127个像素使用相同颜色
				chunkheader -= 127;
				in.read(reinterpret_cast<char*>(colorbuffer.bgra), bpp);
				if (!in.good()) {
					std::cerr << "读取数据时发生错误\n";
					return false;
				}
				for (int i = 0; i < chunkheader; i++) {
					for (int t = 0; t < bpp; t++)
						data[currentbyte++] = colorbuffer.bgra[t];
					currentpixel++;
					if (currentpixel > pixelcount) {
						std::cerr << "读取的像素过多\n";
						return false;
					}
				}
			}
		} while (currentpixel < pixelcount);
		return true;
	}

	// 将图像写入TGA文件
	bool TGAImage::write_tga_file(const std::string filename, const bool vflip, const bool rle) const {
		// TGA文件扩展区域和页脚（用于TGA 2.0格式）
		constexpr std::uint8_t developer_area_ref[4] = { 0, 0, 0, 0 };
		constexpr std::uint8_t extension_area_ref[4] = { 0, 0, 0, 0 };
		constexpr std::uint8_t footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };

		std::ofstream out;
		out.open(filename, std::ios::binary);
		if (!out.is_open()) {
			std::cerr << "无法打开文件 " << filename << "\n";
			return false;
		}

		// 准备文件头
		TGAHeader header = {};
		header.bitsperpixel = bpp << 3;
		header.width = w;
		header.height = h;
		header.datatypecode = (bpp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));  // 根据格式和压缩选项选择类型码
		header.imagedescriptor = vflip ? 0x00 : 0x20;  // 第5位控制原点位置：0=左下角，1=左上角

		out.write(reinterpret_cast<const char*>(&header), sizeof(header));
		if (!out.good()) goto err;
		if (!rle) {  // 未压缩
			out.write(reinterpret_cast<const char*>(data.data()), w * h * bpp);
			if (!out.good()) goto err;
		}
		else if (!unload_rle_data(out)) goto err;  // RLE压缩

		// 写入扩展区域和页脚（TGA 2.0格式）
		out.write(reinterpret_cast<const char*>(developer_area_ref), sizeof(developer_area_ref));
		if (!out.good()) goto err;
		out.write(reinterpret_cast<const char*>(extension_area_ref), sizeof(extension_area_ref));
		if (!out.good()) goto err;
		out.write(reinterpret_cast<const char*>(footer), sizeof(footer));
		if (!out.good()) goto err;
		return true;
	err:
		std::cerr << "无法写入TGA文件\n";
		return false;
	}

	// 将数据以RLE格式写入文件
	bool TGAImage::unload_rle_data(std::ofstream& out) const {
		const std::uint8_t max_chunk_length = 128;  // RLE最大数据包长度
		size_t npixels = w * h;
		size_t curpix = 0;
		while (curpix < npixels) {
			size_t chunkstart = curpix * bpp;
			size_t curbyte = curpix * bpp;
			std::uint8_t run_length = 1;
			bool raw = true;  // true=原始包，false=游程包

			// 寻找最佳的数据包类型和长度
			while (curpix + run_length < npixels && run_length < max_chunk_length) {
				bool succ_eq = true;
				for (int t = 0; succ_eq && t < bpp; t++)
					succ_eq = (data[curbyte + t] == data[curbyte + t + bpp]);  // 检查下一个像素是否相同
				curbyte += bpp;
				if (1 == run_length)
					raw = !succ_eq;  // 根据第一个像素对决定包类型
				if (raw && succ_eq) {  // 原始包中遇到重复像素
					run_length--;
					break;
				}
				if (!raw && !succ_eq)  // 游程包中遇到不同像素
					break;
				run_length++;
			}
			curpix += run_length;

			// 写入数据包头部
			out.put(raw ? run_length - 1 : run_length + 127);
			if (!out.good()) return false;

			// 写入数据
			out.write(reinterpret_cast<const char*>(data.data() + chunkstart), (raw ? run_length * bpp : bpp));
			if (!out.good()) return false;
		}
		return true;
	}

	// 获取指定位置的像素颜色
	TGAColor TGAImage::get(const int x, const int y) const {
		if (!data.size() || x < 0 || y < 0 || x >= w || y >= h) return {};  // 边界检查
		TGAColor ret = { 0, 0, 0, 0, bpp };
		const std::uint8_t* p = data.data() + (x + y * w) * bpp;
		for (int i = bpp; i--; ret.bgra[i] = p[i]);
		return ret;
	}

	// 设置指定位置的像素颜色
	void TGAImage::set(int x, int y, const TGAColor& c) {
		if (!data.size() || x < 0 || y < 0 || x >= w || y >= h) return;  // 边界检查
		memcpy(data.data() + (x + y * w) * bpp, c.bgra, bpp);
	}

	// 水平翻转（左右镜像）
	void TGAImage::flip_horizontally() {
		for (int i = 0; i < w / 2; i++)
			for (int j = 0; j < h; j++)
				for (int b = 0; b < bpp; b++)
					std::swap(data[(i + j * w) * bpp + b], data[(w - 1 - i + j * w) * bpp + b]);
	}

	// 垂直翻转（上下镜像）
	void TGAImage::flip_vertically() {
		for (int i = 0; i < w; i++)
			for (int j = 0; j < h / 2; j++)
				for (int b = 0; b < bpp; b++)
					std::swap(data[(i + j * w) * bpp + b], data[(i + (h - 1 - j) * w) * bpp + b]);
	}

	int TGAImage::width() const {
		return w;
	}

	int TGAImage::height() const {
		return h;
	}
}

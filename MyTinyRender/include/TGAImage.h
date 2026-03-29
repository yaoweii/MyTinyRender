#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
namespace YW {
	// TGA文件头结构（使用1字节对齐以确保文件读写正确）
#pragma pack(push,1)
	struct TGAHeader {
		std::uint8_t  idlength = 0;          // 图像ID字段长度
		std::uint8_t  colormaptype = 0;      // 颜色表类型
		std::uint8_t  datatypecode = 0;      // 图像类型码
		std::uint16_t colormaporigin = 0;    // 颜色表起始索引
		std::uint16_t colormaplength = 0;    // 颜色表长度
		std::uint8_t  colormapdepth = 0;     // 颜色表位深
		std::uint16_t x_origin = 0;          // 图像X原点
		std::uint16_t y_origin = 0;          // 图像Y原点
		std::uint16_t width = 0;             // 图像宽度
		std::uint16_t height = 0;            // 图像高度
		std::uint8_t  bitsperpixel = 0;      // 每像素位数
		std::uint8_t  imagedescriptor = 0;   // 图像描述符
	};
#pragma pack(pop)

	// TGA颜色结构（BGRA格式）
	struct TGAColor {
		std::uint8_t bgra[4] = { 0,0,0,0 };  // 蓝绿红透明度分量
		std::uint8_t bytespp = 4;          // 每像素字节数
		std::uint8_t& operator[](const int i) { return bgra[i]; }
		const std::uint8_t& operator[](const int i) const { return bgra[i]; }
	};

	// TGA图像类，用于加载、保存和处理TGA格式图像
	struct TGAImage {
		enum Format { GRAYSCALE = 1, RGB = 3, RGBA = 4 }; // 支持的图像格式

		TGAImage() = default;
		TGAImage(const int w, const int h, const int bpp, TGAColor c = {}); // 创建指定大小的图像

		bool  read_tga_file(const std::string filename);                     // 从文件读取TGA图像
		bool write_tga_file(const std::string filename, const bool vflip = true,
			const bool rle = true) const;                       // 写入TGA图像到文件
		void flip_horizontally();                                            // 水平翻转
		void flip_vertically();                                              // 垂直翻转
		TGAColor get(const int x, const int y) const;                        // 获取像素颜色
		void set(const int x, const int y, const TGAColor& c);               // 设置像素颜色
		int width()  const;                                                  // 获取图像宽度
		int height() const;                                                  // 获取图像高度
	private:
		bool   load_rle_data(std::ifstream& in);                            // 加载RLE压缩数据
		bool unload_rle_data(std::ofstream& out) const;                     // 写入RLE压缩数据
		int w = 0, h = 0;                                                   // 图像宽高
		std::uint8_t bpp = 0;                                               // 每像素字节数
		std::vector<std::uint8_t> data = {};                                // 像素数据
	};
}
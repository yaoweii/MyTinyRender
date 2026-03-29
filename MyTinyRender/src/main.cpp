#include "../include/shader.h"
#include <iostream>
using namespace YW;
int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "用法: " << argv[0] << " obj/model.obj" << std::endl;
		return 1;
	}

	// 渲染参数设置
	int width = 800;      // 输出图像宽度
	int height = 800;      // 输出图像高度
	Vector3 light{ 1, 1, 1 }; // 光源方向
	Vector3 camera_pos{ 0, 0, 1 }; // 相机位置
	Vector3 target{ 0, 0, 0 }; // 相机朝向目标点
	Vector3 up{ 0, 1, 0 }; // 相机上方向

	// 初始化变换矩阵
	lookat(camera_pos, target, up);                                   // 构建模型视图矩阵
	init_perspective((camera_pos - target).length());             // 构建透视投影矩阵
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // 构建视口矩阵（留出边框）
	init_zbuffer(width, height);                               // 初始化深度缓冲区

	// 创建帧缓冲区（输出图像），浅蓝色背景
	TGAImage framebuffer(width, height, TGAImage::RGB, { 177, 195, 209, 255 });

	// 渲染所有输入模型
	for (int m = 1; m < argc; m++) {
		Model model(argv[m]);      // 加载模型数据
		PhongShader shader(light, model);
		// 遍历模型的所有三角形面
		for (int f = 0; f < model.face_size(); f++) {
			// 组装三角形图元（调用顶点着色器）
			Triangle clip = { shader.vertex_shader(f, 0),
							  shader.vertex_shader(f, 1),
							  shader.vertex_shader(f, 2) };
			rasterize(clip, shader, framebuffer);  // 光栅化三角形
		}
	}

	framebuffer.write_tga_file("framebuffer.tga");  // 保存渲染结果
	return 0;
}

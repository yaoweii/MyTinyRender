// Force recompile
#include "../include/basic_ray_tracing.h"
#include <iostream>
#include <chrono>
#include <omp.h>
#include <deque>

std::vector<std::string> render_target = { "D:\\diablo3_pose\\bunny\\bunny.obj" ,"D:\\diablo3_pose\\bunny\\floor.obj" };

using namespace YW;

// 生成从相机出发经过像素(x,y)的光线
Ray generate_ray(const Vector3& camera_pos, const Vector3& target, const Vector3& up,
	int x, int y, int width, int height, double fov = 60.0) {

	// 计算相机坐标系
	Vector3 forward = (target - camera_pos).normalized();
	Vector3 right = (up % forward).normalized();
	Vector3 camera_up = up.normalized();

	// 计算视平面参数
	double aspect = static_cast<double>(width) / height;
	double fov_rad = fov * 3.1415926535 / 180.0;
	double tan_half_fov = std::tan(fov_rad / 2.0);

	// 归一化设备坐标到[-1,1]
	double ndc_x = (2.0 * (x + 0.5) / width) - 1.0;
	double ndc_y = 1.0 - (2.0 * (y + 0.5) / height);

	// 转换到相机空间
	double camera_x = ndc_x * aspect * tan_half_fov;
	double camera_y = ndc_y * tan_half_fov;

	// 光线方向
	Vector3 direction = forward + right * camera_x + camera_up * camera_y;
	direction = direction.normalized();

	return Ray(camera_pos, direction);
}

int main() {
	// 渲染参数设置
	int width = 800;
	int height = 800;

	std::cout << "正在加载模型..." << std::endl;

	// 创建场景并加载模型
	Scene scene;
	std::deque<Model> models;  // 使用deque避免vector重新分配导致指针失效

	for (int m = 0; m < render_target.size(); m++) {
		models.emplace_back(render_target[m]);
		scene.add_model(&models.back());
		std::cout << "已加载: " << render_target[m] << std::endl;
	}

	// 计算模型包围盒
	Vector3 bbox_min(1e30, 1e30, 1e30);
	Vector3 bbox_max(-1e30, -1e30, -1e30);
	for (const auto& model : models) {
		for (int i = 0; i < model.vert_size(); i++) {
			Vector4 v = model.get_vertex(i);
			Vector3 p = v.xyz();
			bbox_min.x = std::min(bbox_min.x, p.x);
			bbox_min.y = std::min(bbox_min.y, p.y);
			bbox_min.z = std::min(bbox_min.z, p.z);
			bbox_max.x = std::max(bbox_max.x, p.x);
			bbox_max.y = std::max(bbox_max.y, p.y);
			bbox_max.z = std::max(bbox_max.z, p.z);
		}
	}

	// 根据模型大小调整相机位置 - 使用更近的距离
	Vector3 camera_pos(0, 3, 4);  // 从正前方看
	Vector3 target(0, 0, 0);  // 看向模型中心
	Vector3 up{ 0, 1, 0 };
	double fov = 60;  // 使用稍小的视场角，让物体看起来更大

	// 设置点光源位置（在相机上方和后方）
	Vector3 light_pos = Vector3(3, 3, 3);  // bunny
	//Vector3 light_pos = camera_pos + Vector3(-1, 2, 0.5);  // 相机左侧、上方2个单位、后方0.5个单位
	std::cout << "点光源位置: (" << light_pos.x << ", " << light_pos.y << ", " << light_pos.z << ")" << std::endl << std::endl;

	// 构建BVH加速结构
	std::cout << "正在构建BVH加速结构..." << std::endl;
	auto bvh_start = std::chrono::high_resolution_clock::now();
	scene.build_accelerator();
	auto bvh_end = std::chrono::high_resolution_clock::now();
	auto bvh_duration = std::chrono::duration_cast<std::chrono::milliseconds>(bvh_end - bvh_start);
	std::cout << "BVH构建完成，用时: " << bvh_duration.count() / 1000.0 << " 秒" << std::endl;

	Ray test_ray(Vector3(0, 0, 2), Vector3(0, 0, -1));
	Hit ret = scene.bvh.intersect(test_ray);

	// 创建光线追踪着色器
	BasicRayTracingShader shader(scene, light_pos, camera_pos);

	// 创建帧缓冲区（使用vector存储，支持并行写入）
	std::vector<TGAColor> framebuffer_data(width * height, TGAColor{ 0, 0, 0, 255 });

	std::cout << "开始光线追踪渲染..." << std::endl;
	std::cout << "分辨率: " << width << "x" << height << std::endl;

	// 获取OpenMP线程数
	int num_threads = omp_get_max_threads();
	std::cout << "使用线程数: " << num_threads << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();

	// 使用OpenMP并行化像素渲染
	int progress = 0;
	int hit_count = 0;
#pragma omp parallel for schedule(dynamic, 4)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			// 生成光线
			Ray ray = generate_ray(camera_pos, target, up, x, y, width, height, fov);

			// 追踪光线
			TGAColor color = shader.trace(ray);

			// 统计击中数
			if (color.bgra[0] != 177 || color.bgra[1] != 195 || color.bgra[2] != 209) {
#pragma omp atomic
				hit_count++;
			}

			// 写入帧缓冲区
			framebuffer_data[y * width + x] = color;
		}

		// 显示进度（使用atomic避免竞争条件）
#pragma omp atomic capture
		{
			int current_progress = (y * 100) / height;
			if (current_progress > progress) {
				progress = current_progress;
				std::cout << "\r进度: " << progress << "%" << std::flush;
			}
		}
	}

	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

	std::cout << std::endl;
	std::cout << "渲染完成！用时: " << duration.count() / 1000.0 << " 秒" << std::endl;
	std::cout << "击中物体的像素数: " << hit_count << " / " << (width * height) << std::endl;

	// 将数据复制到TGAImage并保存
	TGAImage framebuffer(width, height, TGAImage::RGB);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			framebuffer.set(x, y, framebuffer_data[x + y * width]);
		}
	}

	// 保存结果（不翻转，因为帧缓冲区已经是从上到下的顺序）
	framebuffer.write_tga_file("framebuffer_raytracing.tga", false);
	std::cout << "结果已保存到: framebuffer_raytracing.tga" << std::endl;

	return 0;
}

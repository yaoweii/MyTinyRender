#include "../include/shader.h"
#include <algorithm>
namespace YW {
	// OpenGL状态变量
	Matrix<4, 4> ModelView,   // 模型视图矩阵：将世界坐标变换到相机坐标系
		Perspective,  // 透视投影矩阵：实现透视投影效果
		Viewport;     // 视口矩阵：将归一化设备坐标变换到屏幕坐标

	std::vector<double> zbuffer;  // 深度缓冲区：用于深度测试，消除隐藏面

	// 构建视图矩阵（lookat变换）
	// camera_pos: 相机位置, target: 观察目标点, up: 上方向
	void lookat(const Vector3& camera_pos, const Vector3& target, const Vector3& up) {
		// n: 相机观察方向（从目标指向相机，即-z轴）
		Vector3 n = (camera_pos - target).normalized();
		// l: 右方向向量（x轴）
		Vector3 l = (up % n).normalized();
		// m: 上方向向量（y轴），重新计算以确保正交
		Vector3 m = (n % l).normalized();

		// 构建模型视图矩阵：旋转矩阵 * 平移矩阵
		Matrix<4, 4> M = { {{1, 0, 0, -camera_pos.x}, { 0,1,0,-camera_pos.y }, { 0,0,1,-camera_pos.z }, { 0,0,0,1 }} };
		Matrix<4, 4> R = { {{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}} };
		ModelView = R * M;
	}

	// 初始化透视投影矩阵
	// f: 焦距，控制透视程度
	void init_perspective(const double f) {
		// 简化的透视投影矩阵，将透视除法编码到w分量中
		Perspective = { {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1 / f,1}} };
	}

	// 初始化视口变换矩阵
	// (x,y): 视口左上角, w,h: 视口宽高
	void init_viewport(const int x, const int y, const int w, const int h) {
		// 将[-1,1]的NDC坐标映射到[x,x+w]×[y,y+h]的屏幕坐标
		Matrix<4, 4> M = { {{1, 0, 0, x + w / 2.0}, {0, 1, 0, y + h / 2.0}, {0,0,1,0}, {0,0,0,1}} };
		Matrix<4, 4> S = { {{w / 2.0, 0, 0,0}, {0, h / 2.0, 0, 0}, {0,0,1,0}, {0,0,0,1}} };
		Viewport = M * S;
		//Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
	}

	// 初始化深度缓冲区
	// width,height: 图像分辨率
	void init_zbuffer(const int width, const int height) {
		zbuffer = std::vector<double>(width * height, -1000);  // 初始化为很小的值（最远距离）
	}

	// 光栅化函数：将裁剪空间的三角形绘制到帧缓冲区
	// clip: 裁剪空间的三角形, shader: 着色器, framebuffer: 输出图像
	void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer) {
		// 1. 透视除法：将裁剪坐标转换为归一化设备坐标(NDC)
		Vector4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };

		// 2. 视口变换：将NDC转换为屏幕坐标
		Vector2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() };

		// 3. 构建重心坐标计算矩阵
		Matrix<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.},
						 {screen[1].x, screen[1].y, 1.},
						 {screen[2].x, screen[2].y, 1.} } };
		double det = ABC.det();
		if (det <= 0) {
			return; // 背面剔除 
		}
		if (fabs(det) < 1) { // 面积太小剔除（<0.5像素）
			return;
		}

		// 4. 计算三角形的包围盒
		std::pair<double, double> x_range = std::minmax({ screen[0].x, screen[1].x, screen[2].x });
		std::pair<double, double> y_range = std::minmax({ screen[0].y, screen[1].y, screen[2].y });

		// 5. 遍历包围盒内的每个像素（使用OpenMP并行加速）
#pragma omp parallel for
		for (int x = std::max<int>(x_range.first, 0); x <= std::min<int>(x_range.second, framebuffer.width() - 1); x++) {
			for (int y = std::max<int>(y_range.first, 0); y <= std::min<int>(y_range.second, framebuffer.height() - 1); y++) {
				// 6. 计算像素重心坐标（相对于屏幕空间的三角形）
				Vector3 bc_screen = ABC.invert_transpose() * Vector3 { static_cast<double>(x), static_cast<double>(y), 1. };

				// 7. 透视校正：处理透视变形下的插值
				// 参考：https://github.com/ssloy/tinyrenderer/wiki/Technical-difficulties-linear-interpolation-with-perspective-deformations
				Vector3 bc_clip = { bc_screen.x / clip[0].w, bc_screen.y / clip[1].w, bc_screen.z / clip[2].w };
				bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);

				// 8. 判断像素是否在三角形内
				if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;  // 负的重心坐标表示在三角形外

				// 9. 深度插值
				double z = bc_screen * Vector3{ ndc[0].z, ndc[1].z, ndc[2].z };

				// 10. 深度测试：如果当前像素比z-buffer中的值更近，则绘制
				if (z <= zbuffer[x + y * framebuffer.width()]) continue;

				// 11. 执行片段着色器
				std::pair<bool, TGAColor> ret = shader.pixel_shader(bc_clip);
				if (ret.first) continue;  // 着色器可以选择丢弃此片段(alpha测试)

				// 12. 更新深度缓冲区和帧缓冲区
				zbuffer[x + y * framebuffer.width()] = z;
				framebuffer.set(x, y, ret.second);
			}
		}
	}

}


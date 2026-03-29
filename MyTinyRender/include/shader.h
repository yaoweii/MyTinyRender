#pragma once
#include "geometry.h"
#include "TGAImage.h"
#include "model.h"

namespace YW {
	// OpenGL状态变量
	extern Matrix<4, 4> ModelView,   // 模型视图矩阵：将世界坐标变换到相机坐标系
		Perspective,  // 透视投影矩阵：实现透视投影效果
		Viewport;     // 视口矩阵：将归一化设备坐标变换到屏幕坐标

	extern std::vector<double> zbuffer;  // 深度缓冲区：用于深度测试，消除隐藏面

	// 构建视图矩阵：指定相机位置、观察中心和上方向
	void lookat(const Vector3& camera_pos, const Vector3& target, const Vector3& up);

	// 初始化透视投影矩阵，f为焦距
	void init_perspective(const double f);

	// 初始化视口变换矩阵，将归一化设备坐标映射到屏幕坐标
	void init_viewport(const int x, const int y, const int w, const int h);

	// 初始化Z缓冲区（深度缓冲区），用于深度测试
	void init_zbuffer(const int width, const int height);

	// 着色器接口类，定义了片段着色器的接口
	struct IShader {
		// 2D纹理采样：根据UV坐标从纹理图像中采样颜色
		static TGAColor sample2D(const TGAImage& img, const Vector2& uvf) {
			return img.get(uvf[0] * img.width(), uvf[1] * img.height());
		}

		// 片段着色器：根据重心坐标计算像素颜色
		// 返回值：first表示是否丢弃该像素，second为像素颜色
		virtual std::pair<bool, TGAColor> pixel_shader(const Vector3 bar) const = 0;
	};

	// 三角形类型：由三个有序点组成的图元
	using Triangle = Vector4[3];

	// 光栅化函数：将三角形转换为屏幕像素
	void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer);

	// Phong着色器：实现Phong光照模型
	struct PhongShader : IShader {
		const Model& model;
		Vector4 l;              // 光照方向（在相机坐标系中）
		Vector2 varying_uv[3]; // 每个顶点的纹理坐标，由顶点着色器写入，片段着色器读取
		Vector4 varying_nrm[3]; // 每个顶点的法向量，将被片段着色器插值
		Vector4 tri[3];         // 视图坐标系中的三角形顶点

		PhongShader(const Vector3 light, const Model& m) : model(m) {
			// 将光照方向变换到视图（相机）坐标系
			l = (ModelView * Vector4{ light.x, light.y, light.z, 0. }).normalized();
		}

		// 顶点着色器：处理每个顶点
		virtual Vector4 vertex_shader(const int face, const int vert) {
			varying_uv[vert] = model.get_uv(face, vert);  // 获取纹理坐标
			// 法向量需要用法变换矩阵的逆转置来变换（保证正确处理非均匀缩放）
			varying_nrm[vert] = ModelView.invert_transpose() * model.get_normal(face, vert);
			Vector4 gl_Position = ModelView * model.get_face_vertex(face, vert);  // 变换到视图空间
			tri[vert] = gl_Position;
			return Perspective * gl_Position;  // 应用透视投影，得到裁剪坐标
		}

		// 片段着色器：处理每个像素
		virtual std::pair<bool, TGAColor> pixel_shader(const Vector3 bar) const {
			// 构建Darboux切线空间基（用于正确读取法线贴图）
			Matrix<2, 4> E = { tri[1] - tri[0], tri[2] - tri[0] };  // 三角形边向量
			Matrix<2, 2> U = { varying_uv[1] - varying_uv[0], varying_uv[2] - varying_uv[0] };  // UV差分
			Matrix<2, 4> T = U.invert() * E;  // 从UV空间到视图空间的变换
			Matrix<4, 4> D = { T[0].normalized(),  // 切线向量
				T[1].normalized(),  // 副切线向量
				(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]).normalized(),  // 插值后的法向量
			{ 0,0,0,1 }
			};  // Darboux标架

			// 插值UV坐标
			Vector2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];

			// 从法线贴图读取法向量并变换到视图空间
			Vector4 n = (D.transpose() * model.get_normal(uv)).normalized();

			// 计算反射方向（用于镜面反射）
			Vector4 r = (n * (n * l) * 2 - l).normalized();

			// Phong光照模型的三个分量
			double ambient = 0.4;  // 环境光强度
			double diffuse = 1.0 * std::max(0.0, n * l);  // 漫反射强度（Lambert定律）

			// 镜面反射强度（Phong反射模型）
			// 注意：相机位于z轴上（在相机坐标系中），所以直接用r.z
			TGAColor specular_color = sample2D(model.get_specular(), uv);
			double specular = 0.5 * (specular_color[0] / 255.0) * std::pow(std::max(r.z, 0.0), 50);

			// 从漫反射贴图采样基础颜色
			TGAColor gl_FragColor = sample2D(model.get_diffuse(), uv);

			// 应用光照模型（环境光 + 漫反射 + 镜面反射）
			for (int channel : {0, 1, 2})
				gl_FragColor[channel] = std::min<int>(255, gl_FragColor[channel] * (ambient + diffuse + specular));

			return { false, gl_FragColor };  // 不丢弃此像素
		}
	};
}
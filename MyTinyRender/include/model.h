#pragma once
#include "geometry.h"
#include "TGAImage.h"

namespace YW {
	// 3D模型类，用于从.obj文件加载模型数据
	class Model {
		std::vector<Vector4> verts = {};    // 顶点数组
		std::vector<Vector4> norms = {};    // 法向量数组
		std::vector<Vector2> tex = {};      // 纹理坐标数组
		// 注意：上述数组大小通常不同
		// 可查看Model()构造函数的日志

		std::vector<int> facet_vert_idx = {}; // 每个三角形的顶点索引
		std::vector<int> facet_norm_idx = {}; // 每个三角形的法向量索引
		std::vector<int> facet_tex_idx = {}; // 每个三角形的纹理坐标索引
		// 大小为 nfaces()*3

		TGAImage diffuse_map = {};       // 漫反射贴图（颜色纹理）
		TGAImage normal_map = {};       // 法线贴图
		TGAImage specular_map = {};       // 高光贴图

	public:
		Model(const std::string& filename);  // 从.obj文件构造模型

		int vert_size() const;                 // 顶点数量
		int face_size() const;                 // 三角形面数量

		Vector4 get_vertex(const int i) const;                          // 获取索引i的顶点，0 <= i < nverts()
		Vector4 get_face_vertex(const int iface, const int i) const;   // 获取指定面的第i个顶点，0 <= iface < nfaces(), 0 <= i < 3
		Vector4 get_normal(const int iface, const int i) const; // 获取从.obj文件的"vn x y z"条目读取的法向量
		Vector4 get_normal(const Vector2& uv) const;                     // 从法线贴图获取法向量
		Vector2 get_uv(const int iface, const int i) const;     // 获取三角形某一点的纹理坐标

		const TGAImage& get_diffuse() const;   // 获取漫反射贴图
		const TGAImage& get_specular() const;  // 获取高光贴图
	};
}
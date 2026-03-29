#include <fstream>
#include <sstream>
#include <iostream>
#include "../include/model.h"

namespace YW {
	// 从.obj文件加载3D模型
	Model::Model(const std::string& filename) {
		std::ifstream in;
		in.open(filename, std::ifstream::in);
		if (in.fail()) return;
		std::string line;
		while (!in.eof()) {
			std::getline(in, line);
			std::istringstream iss(line.c_str());
			char trash;
			// 解析顶点数据 "v x y z"
			if (!line.compare(0, 2, "v ")) {
				iss >> trash;
				Vector4 v = { 0,0,0,1 };  // 齐次坐标，w=1表示点
				for (int i = 0; i < 3; i++) {
					iss >> v[i];
				}
				this->verts.push_back(v);
			}
			// 解析法向量数据 "vn x y z"
			else if (!line.compare(0, 3, "vn ")) {
				iss >> trash >> trash;
				Vector4 n;
				for (int i = 0; i < 3; i++) {
					iss >> n[i];
				}
				this->norms.push_back(n.normalized());  // 存储归一化的法向量
			}
			// 解析纹理坐标 "vt u v"
			else if (!line.compare(0, 3, "vt ")) {
				iss >> trash >> trash;
				Vector2 uv;
				for (int i = 0; i < 2; i++) {
					iss >> uv[i];
				}
				this->tex.push_back({ uv.x, 1 - uv.y });  // TGA图像原点在左上角，需要翻转V坐标
			}
			// 解析面数据 "f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3"
			else if (!line.compare(0, 2, "f ")) {
				int v, vt, n, cnt = 0;
				iss >> trash;
				while (iss >> v >> trash >> vt >> trash >> n) {
					this->facet_vert_idx.push_back(--v);  // .obj文件索引从1开始，C++从0开始
					this->facet_tex_idx.push_back(--vt);
					this->facet_norm_idx.push_back(--n);
					cnt++;
				}
				if (3 != cnt) {
					std::cerr << "错误：obj文件应该是三角面化的" << std::endl;
					return;
				}
			}
		}
		std::cerr << "# 顶点数: " << this->vert_size() << " 面数: " << this->face_size() << std::endl;

		// Lambda函数：加载纹理贴图
		auto load_texture = [&filename](const std::string suffix, TGAImage& img) {
			size_t dot = filename.find_last_of(".");
			if (dot == std::string::npos) return;
			std::string texfile = filename.substr(0, dot) + suffix;
			std::cerr << "纹理文件 " << texfile << " 加载" << (img.read_tga_file(texfile.c_str()) ? "成功" : "失败") << std::endl;
			};
		load_texture("_diffuse.tga", this->diffuse_map);   // 加载漫反射贴图
		load_texture("_nm_tangent.tga", this->normal_map);     // 加载法线贴图（切线空间）
		load_texture("_spec.tga", this->specular_map);   // 加载高光贴图
	}

	// 返回顶点数量
	int Model::vert_size() const {
		return this->verts.size();
	}

	// 返回三角形面数量
	int Model::face_size() const {
		return this->facet_vert_idx.size() / 3;
	}

	// 返回索引i处的顶点
	Vector4 Model::get_vertex(const int i) const {
		return this->verts[i];
	}

	// 返回指定面的第i个顶点
	Vector4 Model::get_face_vertex(const int iface, const int i) const {
		int vertex_idx = this->facet_vert_idx[iface * 3 + i];
		assert(vertex_idx >= 0 && vertex_idx < vert_size());
		return this->verts[vertex_idx];
	}

	// 返回指定面的第i个法向量
	Vector4 Model::get_normal(const int iface, const int i) const {
		int normal_idx = this->facet_norm_idx[iface * 3 + i];
		assert(normal_idx >= 0 && normal_idx < norms.size());
		return this->norms[normal_idx];
	}

	// 从法线贴图中获取法向量（在切线空间中）
	Vector4 Model::get_normal(const Vector2& uv) const {
		TGAColor c = this->normal_map.get(uv[0] * this->normal_map.width(), uv[1] * this->normal_map.height());

		// 法线贴图存储的法向量范围是[0,255]，需要转换到[-1,1]
		double R = c[2] * 2.0 / 255.0;
		double G = c[1] * 2.0 / 255.0;
		double B = c[0] * 2.0 / 255.0;
		Vector4 ret = Vector4(R, G, B, 0) - Vector4(1, 1, 1, 0);
		return ret.normalized();
	}

	Vector2 Model::get_uv(const int iface, const int i) const {
		return tex[facet_tex_idx[iface * 3 + i]];  // 返回指定面的第i个点的纹理坐标
	}

	const TGAImage& Model::get_diffuse()  const { return diffuse_map; }  // 获取漫反射贴图
	const TGAImage& Model::get_specular() const { return specular_map; }  // 获取高光贴图
}
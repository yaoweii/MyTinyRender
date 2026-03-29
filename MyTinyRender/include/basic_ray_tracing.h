#pragma once
#include "geometry.h"
#include "model.h"
#include "shader.h"
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <memory>
#include <iostream>

namespace YW {

	// 射线结构（前置声明，AABB需要使用）
	struct Ray {
		Vector3 origin;      // 射线起点
		Vector3 direction;   // 射线方向（单位向量）

		Ray() : origin(0, 0, 0), direction(0, 0, 1) {}
		Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.normalized()) {}

		// 获取射线上参数 t 处的点
		Vector3 at(double t) const {
			return origin + direction * t;
		}
	};

	// 轴对齐包围盒（AABB）
	struct AABB {
		Vector3 min;
		Vector3 max;

		AABB() : min(1e30, 1e30, 1e30), max(-1e30, -1e30, -1e30) {}

		AABB(const Vector3& min_, const Vector3& max_) : min(min_), max(max_) {}

		// 扩展包围盒以包含一个点
		void expand(const Vector3& point) {
			min.x = std::min(min.x, point.x);
			min.y = std::min(min.y, point.y);
			min.z = std::min(min.z, point.z);
			max.x = std::max(max.x, point.x);
			max.y = std::max(max.y, point.y);
			max.z = std::max(max.z, point.z);
		}

		// 扩展包围盒以包含另一个包围盒
		void expand(const AABB& other) {
			min.x = std::min(min.x, other.min.x);
			min.y = std::min(min.y, other.min.y);
			min.z = std::min(min.z, other.min.z);
			max.x = std::max(max.x, other.max.x);
			max.y = std::max(max.y, other.max.y);
			max.z = std::max(max.z, other.max.z);
		}

		// 计算包围盒表面积（用于SAH分割启发式）
		double surface_area() const {
			Vector3 extent = max - min;
			return 2.0 * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
		}

		// 光线与AABB相交测试（AABB slab算法）
		bool intersect(const Ray& ray) const {
			// 计算倒数方向（处理分量为0的情况）
			double inv_dx = (ray.direction.x != 0.0) ? 1.0 / ray.direction.x : std::numeric_limits<double>::infinity();
			double inv_dy = (ray.direction.y != 0.0) ? 1.0 / ray.direction.y : std::numeric_limits<double>::infinity();
			double inv_dz = (ray.direction.z != 0.0) ? 1.0 / ray.direction.z : std::numeric_limits<double>::infinity();

			// 逐元素计算 t0 和 t1
			double t0x = (min.x - ray.origin.x) * inv_dx;
			double t0y = (min.y - ray.origin.y) * inv_dy;
			double t0z = (min.z - ray.origin.z) * inv_dz;

			double t1x = (max.x - ray.origin.x) * inv_dx;
			double t1y = (max.y - ray.origin.y) * inv_dy;
			double t1z = (max.z - ray.origin.z) * inv_dz;

			// 确保 t0 <= t1（处理无穷大情况）
			if (t0x > t1x) std::swap(t0x, t1x);
			if (t0y > t1y) std::swap(t0y, t1y);
			if (t0z > t1z) std::swap(t0z, t1z);

			// 计算进入和离开点
			double t_min = std::max({ t0x, t0y, t0z });
			double t_max = std::min({ t1x, t1y, t1z });

			// 如果 t_max < t_min 或者 t_max < 0，则不相交
			return t_max >= t_min && t_max >= 1e-6;
		}

		// 计算包围盒中心点
		Vector3 center() const {
			return (min + max) * 0.5;
		}

		// 获取最长轴（用于分割）
		int longest_axis() const {
			Vector3 extent = max - min;
			if (extent.x > extent.y && extent.x > extent.z) return 0;
			if (extent.y > extent.z) return 1;
			return 2;
		}
	};

	// 三角形图元（用于BVH构建）
	struct TrianglePrimitive {
		Vector3 v0, v1, v2;	//点
		Vector3 n0, n1, n2;	//法向
		Vector2 uv0, uv1, uv2;	//uv
		Vector3 tangent;   // 切线向量（用于法线贴图）
		const Model* model;
		int face_idx;

		TrianglePrimitive() : model(nullptr), face_idx(-1), tangent(0, 0, 0) {}

		AABB get_bounds() const {
			AABB bounds;
			bounds.expand(v0);
			bounds.expand(v1);
			bounds.expand(v2);
			return bounds;
		}

		Vector3 center() const {
			return (v0 + v1 + v2) * (1.0 / 3.0);
		}
	};

	// 击中信息结构
	struct Hit {
		bool hit;           // 是否击中
		double t;           // 击中点的参数值
		Vector3 point;      // 击中点坐标
		Vector3 normal;     // 击中点法向量
		Vector3 tangent;    // 切线向量（用于法线贴图）
		Vector2 uv;         // 纹理坐标
		int face_idx;       // 击中的面索引
		const Model* model; // 击中的模型

		Hit() : hit(false), t(std::numeric_limits<double>::max()), point(0, 0, 0),
			normal(0, 0, 0), tangent(0, 0, 0), uv(0, 0), face_idx(-1), model(nullptr) {
		}
	};

	// 光线与三角形的相交测试（Möller-Trumbore 算法）
	// 需要在BVH类之前定义，因为BVH::intersect_recursive使用此函数
	inline bool ray_triangle_intersect(const Ray& ray, const Vector3& v0, const Vector3& v1, const Vector3& v2, double& t, double& u, double& v) {
		Vector3 edge1 = v1 - v0;
		Vector3 edge2 = v2 - v0;
		Vector3 h = ray.direction % edge2;  // 修正：使用 edge2，不是 edge1！
		double a = edge1 * h;

		if (std::fabs(a) < 1e-10) return false;  // 光线平行于三角形

		double f = 1.0 / a;
		Vector3 s = ray.origin - v0;
		u = f * (s * h);

		if (u < 0.0 || u > 1.0) return false;

		Vector3 q = s % edge1;
		v = f * (ray.direction * q);

		if (v < 0.0 || u + v > 1.0) return false;

		t = f * (edge2 * q);

		return t > 1e-6;  // 避免自身相交
	}

	// BVH节点
	struct BVHNode {
		AABB bounds;
		std::unique_ptr<BVHNode> left;
		std::unique_ptr<BVHNode> right;
		std::vector<TrianglePrimitive> primitives;  // 叶子节点的图元
		bool is_leaf;

		BVHNode() : is_leaf(false) {}

		// 判断是否为叶子节点
		bool leaf() const {
			return is_leaf;
		}
	};

	// BVH树
	class BVH {
	public:
		std::unique_ptr<BVHNode> root;
		int max_primitives_per_node = 4;  // 每个叶子节点最大图元数
		int max_depth = 20;                // 最大递归深度

	public:
		BVH() : root(nullptr) {}

		// 从场景构建BVH
		void build(const std::vector<const Model*>& models) {
			std::vector<TrianglePrimitive> primitives;

			// 收集所有三角形
			for (const Model* model : models) {
				for (int f = 0; f < model->face_size(); f++) {
					TrianglePrimitive prim;
					prim.v0 = model->get_face_vertex(f, 0).xyz();
					prim.v1 = model->get_face_vertex(f, 1).xyz();
					prim.v2 = model->get_face_vertex(f, 2).xyz();
					prim.n0 = model->get_normal(f, 0).xyz();
					prim.n1 = model->get_normal(f, 1).xyz();
					prim.n2 = model->get_normal(f, 2).xyz();
					prim.uv0 = model->get_uv(f, 0);
					prim.uv1 = model->get_uv(f, 1);
					prim.uv2 = model->get_uv(f, 2);
					prim.model = model;
					prim.face_idx = f;

					// 计算切线向量（用于法线贴图）
					Vector3 edge1 = prim.v1 - prim.v0;
					Vector3 edge2 = prim.v2 - prim.v0;
					Vector2 deltaUV1 = prim.uv1 - prim.uv0;
					Vector2 deltaUV2 = prim.uv2 - prim.uv0;

					double det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
					if (std::fabs(det) > 1e-8) {
						Vector3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) / det;
						prim.tangent = tangent.normalized();
					}
					else {
						prim.tangent = Vector3(1, 0, 0);  // 默认值
					}

					primitives.push_back(prim);
				}
			}

			std::cerr << "总共收集到三角形: " << primitives.size() << std::endl;

			if (primitives.empty()) {
				root = nullptr;
				return;
			}

			// 递归构建BVH
			root = std::unique_ptr<BVHNode>(new BVHNode());
			build_recursive(root.get(), primitives, 0);
		}

		// 光线与BVH求交（返回最近击中点）
		Hit intersect(const Ray& ray) const {
			Hit closest;
			closest.t = std::numeric_limits<double>::max();

			if (root) {
				intersect_recursive(root.get(), ray, closest);
			}

			return closest;
		}

	private:
		// 递归构建BVH
		void build_recursive(BVHNode* node, std::vector<TrianglePrimitive>& primitives, int depth) {
			// 计算当前节点的包围盒
			node->bounds = AABB();
			for (const auto& prim : primitives) {
				node->bounds.expand(prim.get_bounds());
			}

			// 如果图元数量少或达到最大深度，创建叶子节点
			if (primitives.size() <= max_primitives_per_node || depth >= max_depth) {
				node->is_leaf = true;
				node->primitives = std::move(primitives);
				return;
			}

			// 选择分割轴（最长轴）
			int axis = node->bounds.longest_axis();

			// 按中心点排序
			std::sort(primitives.begin(), primitives.end(),
				[axis](const TrianglePrimitive& a, const TrianglePrimitive& b) {
					return a.center()[axis] < b.center()[axis];
				});

			// 分割图元
			size_t mid = primitives.size() / 2;
			std::vector<TrianglePrimitive> left_prims(primitives.begin(), primitives.begin() + mid);
			std::vector<TrianglePrimitive> right_prims(primitives.begin() + mid, primitives.end());

			// 创建子节点
			node->left = std::unique_ptr<BVHNode>(new BVHNode());
			node->right = std::unique_ptr<BVHNode>(new BVHNode());

			build_recursive(node->left.get(), left_prims, depth + 1);
			build_recursive(node->right.get(), right_prims, depth + 1);
		}

		// 递归求交
		void intersect_recursive(const BVHNode* node, const Ray& ray, Hit& closest) const {
			// 如果光线不与节点的包围盒相交，直接返回
			bool hit_aabb = node->bounds.intersect(ray);
			if (!hit_aabb) {
				return;
			}

			// 如果是叶子节点，测试所有图元
			if (node->is_leaf) {
				for (const auto& prim : node->primitives) {
					double t, u, v;
					bool hit_tri = ray_triangle_intersect(ray, prim.v0, prim.v1, prim.v2, t, u, v);

					if (hit_tri && t < closest.t) {
						closest.t = t;
						closest.hit = true;
						closest.point = ray.at(t);
						closest.face_idx = prim.face_idx;
						closest.model = prim.model;

						// 插值法向量
						closest.normal = (prim.n0 * (1 - u - v) + prim.n1 * u + prim.n2 * v).normalized();

						// 插值切线向量
						closest.tangent = prim.tangent;  // 三角形的切线是常数

						// 插值纹理坐标
						closest.uv = prim.uv0 * (1 - u - v) + prim.uv1 * u + prim.uv2 * v;
					}
				}
			}
			else {
				// 递归测试子节点（先测试更近的子节点）
				intersect_recursive(node->left.get(), ray, closest);
				intersect_recursive(node->right.get(), ray, closest);
			}
		}
	};

	// 场景中的所有模型
	struct Scene {
		std::vector<const Model*> models;
		BVH bvh;
		bool bvh_built = false;

		void add_model(Model* model) {
			models.push_back(model);
			bvh_built = false;  // 需要重新构建BVH
		}

		// 构建BVH加速结构
		void build_accelerator() {
			if (!bvh_built) {
				bvh.build(models);
				bvh_built = true;
			}
		}

		// 光线与场景求交
		Hit intersect(const Ray& ray) const {
			return bvh.intersect(ray);
		}

		// 阴影射线测试（点光源版本）
		bool is_shadowed_point_light(const Vector3& point, const Vector3& light_pos) const {
			Vector3 light_dir = light_pos - point;
			double light_dist = light_dir.length();
			light_dir = light_dir.normalized();

			Ray shadow_ray(point + light_dir * 1e-4, light_dir);
			Hit hit = intersect(shadow_ray);

			// 如果有遮挡物且距离小于光源距离，则被遮挡
			return hit.hit && hit.t < light_dist - 1e-4;
		}
	};

	// 基础光线追踪着色器
	struct BasicRayTracingShader {
		const Scene& scene;
		Vector3 light_pos;      // 点光源位置
		Vector3 camera_pos;     // 相机位置

		BasicRayTracingShader(const Scene& s, const Vector3& light, const Vector3& cam)
			: scene(s), light_pos(light), camera_pos(cam) {
		}

		// Phong 光照模型（点光源版本，包含法线贴图）
		TGAColor compute_phong(const Hit& hit, const Vector3& view_dir) const {
			if (!hit.model) return TGAColor{ 0, 0, 0, 255 };

			// 从漫反射贴图获取颜色
			const TGAImage& diffuse = hit.model->get_diffuse();
			TGAColor texture_color = IShader::sample2D(diffuse, hit.uv);

			// 从高光贴图获取高光强度
			const TGAImage& specular = hit.model->get_specular();
			double specular_intensity = IShader::sample2D(specular, hit.uv)[0] / 255.0;

			// 使用法线贴图获取法线
			Vector3 normal = hit.model->get_normal(hit.uv).xyz();  // 从法线贴图读取（切线空间）

			// 构建TBN矩阵将切线空间法线转换到世界空间
			Vector3 bitangent = (hit.tangent % hit.normal).normalized();
			Matrix<3, 3> TBN = {
				Vector3(hit.tangent.x, bitangent.x, hit.normal.x),
				Vector3(hit.tangent.y, bitangent.y, hit.normal.y),
				Vector3(hit.tangent.z, bitangent.z, hit.normal.z)
			};

			// 将切线空间法线转换到世界空间
			Vector3 world_normal = TBN * normal;
			normal = world_normal.normalized();

			// 计算从击中点到光源的向量和距离
			Vector3 light_dir = light_pos - hit.point;
			double light_dist = light_dir.length();
			light_dir = light_dir.normalized();

			// 距离衰减（平方反比定律，加了变暗很多，先不加）
			double attenuation = 1.0 /*/ (1.0 + 0.1 * light_dist + 0.01 * light_dist * light_dist)*/;

			// 环境光
			double ambient = 0.15;

			// 漫反射（Lambert）
			double diffuse_factor = std::max(0.0, normal * light_dir);

			// 镜面反射（Blinn-Phong）
			Vector3 half_vector = (light_dir + view_dir).normalized();
			double specular_factor = specular_intensity * std::pow(std::max(0.0, normal * half_vector), 64.0);

			// 阴影测试（点光源）
			double shadow = scene.is_shadowed_point_light(hit.point, light_pos) ? 0.3 : 1.0;

			// 组合光照（应用衰减）
			double intensity = ambient + shadow * attenuation * (diffuse_factor + 0.5 * specular_factor);

			TGAColor result;
			for (int c = 0; c < 3; c++) {
				result[c] = std::min(255, static_cast<int>(texture_color[c] * intensity));
			}
			result[3] = 255;

			return result;
		}

		// 追踪一条光线并返回颜色
		TGAColor trace(const Ray& ray, int depth = 0) const {
			Hit hit = scene.intersect(ray);

			if (!hit.hit) {
				// 未击中任何物体，返回背景色
				return TGAColor{ 177, 195, 209, 255 };  // 浅蓝色背景
			}

			Vector3 view_dir = (camera_pos - hit.point).normalized();
			return compute_phong(hit, view_dir);
		}
	};

}

//#pragma once
//#include "geometry.h"
//#include "model.h"
//#include "basic_ray_tracing.h"
//#include <vector>
//#include <limits>
//#include <cmath>
//
//namespace YW {
//
//	// 材质属性
//	struct Material {
//		double ambient;         // 环境光系数
//		double diffuse;         // 漫反射系数
//		double specular;        // 镜面反射系数
//		double shininess;       // 高光指数
//		double reflectivity;    // 反射率（0 = 不反射，1 = 完全镜面）
//		double transparency;    // 透明度（0 = 不透明，1 = 完全透明）
//		double refractive_index; // 折射率（例如：空气=1.0，玻璃=1.5，水=1.33）
//
//		Material() : ambient(0.15), diffuse(0.7), specular(0.3), shininess(64.0),
//			reflectivity(0.0), transparency(0.0), refractive_index(1.0) {}
//	};
//
//	// 扩展的击中信息，包含材质
//	struct MaterialHit : public Hit {
//		Material material;
//
//		MaterialHit() : Hit(), material() {}
//	};
//
//	// Whited 风格光线追踪着色器
//	struct WhitedStyleRayTracingShader {
//		const Scene& scene;
//		Vector3 light_dir;
//		Vector3 camera_pos;
//		int max_depth;          // 最大递归深度
//
//		WhitedStyleRayTracingShader(const Scene& s, const Vector3& light, const Vector3& cam, int depth = 5)
//			: scene(s), light_dir(light.normalized()), camera_pos(cam), max_depth(depth) {}
//
//		// 计算反射方向
//		Vector3 reflect(const Vector3& incident, const Vector3& normal) const {
//			return incident - normal * 2.0 * (incident * normal);
//		}
//
//		// 计算折射方向（Snell 定律）
//		bool refract(const Vector3& incident, const Vector3& normal, double n1, double n2, Vector3& refracted) const {
//			double eta = n1 / n2;
//			double cos_i = -incident * normal;
//			double sin_t2 = eta * eta * (1.0 - cos_i * cos_i);
//
//			if (sin_t2 > 1.0) return false;  // 全内反射
//
//			double cos_t = std::sqrt(1.0 - sin_t2);
//			refracted = incident * eta + normal * (eta * cos_i - cos_t);
//			return true;
//		}
//
//		// Fresnel 效果（Schlick 近似）
//		double fresnel(const Vector3& incident, const Vector3& normal, double n1, double n2) const {
//			double cos_i = -incident * normal;
//			double r0 = (n1 - n2) / (n1 + n2);
//			r0 = r0 * r0;
//
//			// 处理从内部射出的情况
//			if (n1 > n2) {
//				double eta = n1 / n2;
//				double sin_t2 = eta * eta * (1.0 - cos_i * cos_i);
//				if (sin_t2 > 1.0) return 1.0;  // 全内反射
//				cos_i = std::sqrt(1.0 - sin_t2);
//			}
//
//			double x = 1.0 - cos_i;
//			return r0 + (1.0 - r0) * x * x * x * x * x;
//		}
//
//		// Phong 光照模型（包含阴影）
//		TGAColor compute_phong(const MaterialHit& hit, const Vector3& view_dir) const {
//			if (!hit.model) return TGAColor{ 0, 0, 0, 255 };
//
//			const TGAImage& diffuse = hit.model->get_diffuse();
//			TGAColor texture_color = IShader::sample2D(diffuse, hit.uv);
//
//			const TGAImage& specular_map = hit.model->get_specular();
//			double specular_intensity = IShader::sample2D(specular_map, hit.uv)[0] / 255.0;
//
//			Vector3 normal = hit.normal;
//
//			// 环境光
//			TGAColor result;
//			for (int c = 0; c < 3; c++) {
//				result[c] = static_cast<int>(texture_color[c] * hit.material.ambient);
//			}
//
//			// 阴影测试
//			bool in_shadow = scene.is_shadowed(hit.point, light_dir, std::numeric_limits<double>::max());
//
//			if (!in_shadow) {
//				Vector3 light = light_dir;
//
//				// 漫反射
//				double diffuse_factor = std::max(0.0, normal * light);
//
//				// 镜面反射（Blinn-Phong）
//				Vector3 half_vector = (light + view_dir).normalized();
//				double specular_factor = specular_intensity * hit.material.specular *
//					std::pow(std::max(0.0, normal * half_vector), hit.material.shininess);
//
//				for (int c = 0; c < 3; c++) {
//					result[c] = std::min(255, static_cast<int>(
//						result[c] + texture_color[c] * hit.material.diffuse * diffuse_factor +
//						255 * specular_factor
//					));
//				}
//			}
//
//			result[3] = 255;
//			return result;
//		}
//
//		// 获取默认材质（可以从纹理贴图推断）
//		Material get_material(const Hit& hit) const {
//			Material mat;
//
//			if (!hit.model) return mat;
//
//			// 从高光贴图推断反射率
//			const TGAImage& specular = hit.model->get_specular();
//			double specular_intensity = IShader::sample2D(specular, hit.uv)[0] / 255.0;
//
//			// 高光区域有较强的反射
//			mat.reflectivity = specular_intensity * 0.5;
//			mat.specular = specular_intensity;
//			mat.diffuse = 1.0 - specular_intensity * 0.3;
//
//			return mat;
//		}
//
//		// 递归追踪光线
//		TGAColor trace(const Ray& ray, int depth = 0) const {
//			if (depth > max_depth) {
//				return TGAColor{ 0, 0, 0, 255 };  // 超过最大深度，返回黑色
//			}
//
//			Hit base_hit = scene.intersect(ray);
//			if (!base_hit.hit) {
//				return TGAColor{ 177, 195, 209, 255 };  // 背景色
//			}
//
//			MaterialHit hit;
//			static_cast<Hit&>(hit) = base_hit;
//			hit.material = get_material(base_hit);
//
//			Vector3 view_dir = (camera_pos - hit.point).normalized();
//			TGAColor local_color = compute_phong(hit, view_dir);
//
//			// 递归反射
//			TGAColor reflect_color{ 0, 0, 0, 255 };
//			if (hit.material.reflectivity > 0.01) {
//				Vector3 reflect_dir = reflect(-view_dir, hit.normal).normalized();
//				Ray reflect_ray(hit.point + hit.normal * 1e-4, reflect_dir);
//				reflect_color = trace(reflect_ray, depth + 1);
//
//				// 混合反射色
//				for (int c = 0; c < 3; c++) {
//					local_color[c] = std::min(255, static_cast<int>(
//						local_color[c] * (1.0 - hit.material.reflectivity) +
//						reflect_color[c] * hit.material.reflectivity
//					));
//				}
//			}
//
//			// 递归折射（透明物体）
//			TGAColor refract_color{ 0, 0, 0, 255 };
//			if (hit.material.transparency > 0.01) {
//				Vector3 refract_dir;
//				double n1 = 1.0;  // 空气
//				double n2 = hit.material.refractive_index;
//
//				bool entering = (ray.direction * hit.normal) < 0;
//				Vector3 normal = entering ? hit.normal : -hit.normal;
//
//				if (entering) {
//					// 从空气进入物体
//					if (refract(ray.direction, normal, n1, n2, refract_dir)) {
//						Ray refract_ray(hit.point - normal * 1e-4, refract_dir);
//						refract_color = trace(refract_ray, depth + 1);
//					}
//				} else {
//					// 从物体内部射出
//					if (refract(ray.direction, normal, n2, n1, refract_dir)) {
//						Ray refract_ray(hit.point - normal * 1e-4, refract_dir);
//						refract_color = trace(refract_ray, depth + 1);
//					}
//				}
//
//				// 计算菲涅尔系数
//				double fr = fresnel(ray.direction, normal, n1, n2);
//
//				// 混合折射色和反射色（能量守恒）
//				for (int c = 0; c < 3; c++) {
//					local_color[c] = std::min(255, static_cast<int>(
//						local_color[c] * (1.0 - hit.material.transparency) +
//						refract_color[c] * hit.material.transparency * (1.0 - fr)
//					));
//				}
//			}
//
//			local_color[3] = 255;
//			return local_color;
//		}
//	};
//
//}

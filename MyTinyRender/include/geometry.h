#pragma once
#include <vector>
#include <cassert>
#include <ostream>
#include <cmath>

namespace YW {
#pragma region N维向量定义

	template<int N>
	struct Vector {
		std::vector<double> _vtr;
		Vector() :_vtr(N, 0) {}

		//访问下标
		double& operator[](int idx) {
			assert(idx >= 0 && idx < N);
			return _vtr[idx];
		}

		const double& operator[](int idx) const {
			assert(idx >= 0 && idx < N);
			return _vtr[idx];
		}

		// 返回向量长度
		double length() const {
			double sum = 0.0;
			for (int i = 0; i < N; ++i) sum += _vtr[i] * _vtr[i];
			return std::sqrt(sum);
		}

		// 返回单位化后的向量（不修改原向量）
		Vector<N> normalized() const {
			double len = length();
			assert(len != 0.0);
			Vector<N> ret;
			for (int i = 0; i < N; ++i) ret[i] = _vtr[i] / len;
			return ret;
		}

		// 将向量自身单位化
		void normalize() {
			double len = length();
			assert(len != 0.0);
			for (int i = 0; i < N; ++i) _vtr[i] /= len;
		}
	};

	//点乘
	template<int N>
	double operator*(const Vector<N>& vtr1, const Vector<N>& vtr2) {
		double ret = 0;
		for (int i = 0; i < N; i++) {
			ret += (vtr1[i] * vtr2[i]);
		}
		return ret;
	}

	//向量加法
	template<int N>
	Vector<N> operator+(const Vector<N>& vtr1, const Vector<N>& vtr2) {
		Vector<N> ret;
		for (int i = 0; i < N; i++) {
			ret[i] = vtr1[i] + vtr2[i];
		}
		return ret;
	}

	//向量减法
	template<int N>
	Vector<N> operator-(const Vector<N>& vtr1, const Vector<N>& vtr2) {
		Vector<N> ret;
		for (int i = 0; i < N; i++) {
			ret[i] = vtr1[i] - vtr2[i];
		}
		return ret;
	}

	//向量与标量乘法
	template<int N>
	Vector<N> operator*(const Vector<N>& vtr, double s) {
		Vector<N> ret;
		for (int i = 0; i < N; i++) {
			ret[i] = vtr[i] * s;
		}
		return ret;
	}

	//标量与向量乘法(交换律)
	template<int N>
	Vector<N> operator*(double s, const Vector<N>& vtr) {
		return vtr * s;
	}

	//向量除以标量
	template<int N>
	Vector<N> operator/(const Vector<N>& vtr, double s) {
		assert(s != 0.0);
		Vector<N> ret;
		for (int i = 0; i < N; i++) {
			ret[i] = vtr[i] / s;
		}
		return ret;
	}

	//调试输出重载
	template<int N>
	std::ostream& operator<<(std::ostream& os, const Vector<N>& vtr) {
		os << "(";
		for (int i = 0; i < N; i++) {
			os << vtr[i];
			if (i + 1 < N) os << ", ";
		}
		os << ")";
		return os;
	}
#pragma endregion

#pragma region 二维三维四维向量特化
	// 2D, 3D, 4D 特化：提供命名成员与便捷构造

	// 二维向量特化
	template<>
	struct Vector<2> {
		double x, y;
		Vector() : x(0.0), y(0.0) {}
		Vector(double x_, double y_) : x(x_), y(y_) {}

		double& operator[](int idx) {
			assert(idx >= 0 && idx < 2);
			if (idx == 0) return x;
			return y;
		}

		const double& operator[](int idx) const {
			assert(idx >= 0 && idx < 2);
			if (idx == 0) return x;
			return y;
		}

		// 返回向量长度
		double length() const {
			return (x * x + y * y);
		}

		// 返回单位化后的向量（不修改原向量）
		Vector<2> normalized() const {
			double len = length();
			assert(len != 0.0);
			Vector<2> ret = { x / len , y / len };
			return ret;
		}

		// 将向量自身单位化
		void normalize() {
			double len = length();
			assert(len != 0.0);
			x /= len; y /= len;
		}
	};

	// 三维向量特化
	template<>
	struct Vector<3> {
		double x, y, z;
		Vector() : x(0.0), y(0.0), z(0.0) {}
		Vector(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

		double& operator[](int idx) {
			assert(idx >= 0 && idx < 3);
			if (idx == 0) return x;
			if (idx == 1) return y;
			return z;
		}

		const double& operator[](int idx) const {
			assert(idx >= 0 && idx < 3);
			if (idx == 0) return x;
			if (idx == 1) return y;
			return z;
		}

		// 返回向量长度
		double length() const {
			return std::sqrt(x * x + y * y + z * z);
		}

		// 返回单位化后的向量（不修改原向量）
		Vector<3> normalized() const {
			double len = length();
			assert(len != 0.0);
			Vector<3> ret(x / len, y / len, z / len);
			return ret;
		}

		// 将向量自身单位化
		void normalize() {
			double len = length();
			assert(len != 0.0);
			x /= len; y /= len; z /= len;
		}

		//向量叉乘
		Vector<3> operator%(const Vector<3>& vtr2) const {
			return Vector<3>(
				this->y * vtr2.z - this->z * vtr2.y,
				this->z * vtr2.x - this->x * vtr2.z,
				this->x * vtr2.y - this->y * vtr2.x
			);
		}
	};

	// 四维向量特化
	template<>
	struct Vector<4> {
		double x, y, z, w;
		Vector() : x(0.0), y(0.0), z(0.0), w(0.0) {}
		Vector(double x_, double y_, double z_, double w_) : x(x_), y(y_), z(z_), w(w_) {}

		double& operator[](int idx) {
			assert(idx >= 0 && idx < 4);
			if (idx == 0) return x;
			if (idx == 1) return y;
			if (idx == 2) return z;
			return w;
		}

		const double& operator[](int idx) const {
			assert(idx >= 0 && idx < 4);
			if (idx == 0) return x;
			if (idx == 1) return y;
			if (idx == 2) return z;
			return w;
		}

		// 返回向量长度
		double length() const {
			return (x * x + y * y + z * z + w * w);
		}

		// 返回单位化后的向量（不修改原向量）
		Vector<4> normalized() const {
			double len = length();
			assert(len != 0.0);
			Vector<4> ret = { x / len , y / len , z / len , w / len };
			return ret;
		}

		// 将向量自身单位化
		void normalize() {
			double len = length();
			assert(len != 0.0);
			x /= len; y /= len; z /= len; w /= len;
		}

		Vector<2> xy()  const { return { x, y }; }  // 提取xy分量
		Vector<3> xyz() const { return { x, y, z }; }  // 提取xyz分量
	};

	using Vector2 = Vector<2>;
	using Vector3 = Vector<3>;
	using Vector4 = Vector<4>;

#pragma endregion
#pragma region 矩阵定义
	// 行列式计算的模板元编程辅助结构前置声明
	template<int n> struct dt;

	// 矩阵模板类
	template<int nrows, int ncols>
	struct Matrix {
		Vector<ncols> rows[nrows];

		Vector<ncols>& operator[](const int idx) {
			assert(idx >= 0 && idx < nrows);
			return rows[idx];
		}

		const Vector<ncols>& operator[](const int idx) const {
			assert(idx >= 0 && idx < nrows);
			return rows[idx];
		}

		// 计算行列式
		double det() const {
			return dt<ncols>::det(*this);
		}

		// 计算余子式
		double cofactor(const int row, const int col) const {
			Matrix<nrows - 1, ncols - 1> submatrix;
			for (int i = 0; i < nrows - 1; i++)
				for (int j = 0; j < ncols - 1; j++)
					submatrix[i][j] = rows[i + (i >= row ? 1 : 0)][j + (j >= col ? 1 : 0)];
			return submatrix.det() * ((row + col) % 2 ? -1 : 1);
		}

		// 计算转置的逆矩阵（用于求逆的中间步骤）
		Matrix<nrows, ncols> invert_transpose() const {
			Matrix<nrows, ncols> adjugate_transpose;
			for (int i = 0; i < nrows; i++)
				for (int j = 0; j < ncols; j++)
					adjugate_transpose[i][j] = cofactor(i, j);
			return adjugate_transpose / (adjugate_transpose[0] * rows[0]);
		}

		// 计算逆矩阵
		Matrix<nrows, ncols> invert() const {
			return invert_transpose().transpose();
		}

		// 矩阵转置
		Matrix<ncols, nrows> transpose() const {
			Matrix<ncols, nrows> ret;
			for (int i = 0; i < ncols; i++)
				for (int j = 0; j < nrows; j++)
					ret[i][j] = rows[j][i];
			return ret;
		}
	};

	// 行向量与矩阵乘法
	template<int nrows, int ncols>
	Vector<ncols> operator*(const Vector<nrows>& lhs, const Matrix<nrows, ncols>& rhs) {
		Vector<ncols> ret;
		for (int i = 0; i < ncols; i++)
			ret[i] = lhs * rhs[i];
		return ret;
	}

	// 矩阵与列向量乘法
	template<int nrows, int ncols>
	Vector<nrows> operator*(const Matrix<nrows, ncols>& lhs, const Vector<ncols>& rhs) {
		Vector<nrows> ret;
		for (int i = 0; i < nrows; i++)
			ret[i] = lhs[i] * rhs;
		return ret;
	}

	// 矩阵乘法
	template<int R1, int C1, int C2>
	Matrix<R1, C2> operator*(const Matrix<R1, C1>& lhs, const Matrix<C1, C2>& rhs) {
		Matrix<R1, C2> result;
		for (int i = 0; i < R1; i++)
			for (int j = 0; j < C2; j++)
				for (int k = 0; k < C1; k++)
					result[i][j] += lhs[i][k] * rhs[k][j];
		return result;
	}

	// 矩阵与标量乘法
	template<int nrows, int ncols>
	Matrix<nrows, ncols> operator*(const Matrix<nrows, ncols>& lhs, const double val) {
		Matrix<nrows, ncols> result;
		for (int i = 0; i < nrows; i++)
			result[i] = lhs[i] * val;
		return result;
	}

	// 标量与矩阵乘法(交换律)
	template<int nrows, int ncols>
	Matrix<nrows, ncols> operator*(const double val, const Matrix<nrows, ncols>& rhs) {
		return rhs * val;
	}

	// 矩阵与标量除法
	template<int nrows, int ncols>
	Matrix<nrows, ncols> operator/(const Matrix<nrows, ncols>& lhs, const double val) {
		Matrix<nrows, ncols> result;
		for (int i = 0; i < nrows; i++)
			result[i] = lhs[i] / val;
		return result;
	}

	// 矩阵加法
	template<int nrows, int ncols>
	Matrix<nrows, ncols> operator+(const Matrix<nrows, ncols>& lhs, const Matrix<nrows, ncols>& rhs) {
		Matrix<nrows, ncols> result;
		for (int i = 0; i < nrows; i++)
			for (int j = 0; j < ncols; j++)
				result[i][j] = lhs[i][j] + rhs[i][j];
		return result;
	}

	// 矩阵减法
	template<int nrows, int ncols>
	Matrix<nrows, ncols> operator-(const Matrix<nrows, ncols>& lhs, const Matrix<nrows, ncols>& rhs) {
		Matrix<nrows, ncols> result;
		for (int i = 0; i < nrows; i++)
			for (int j = 0; j < ncols; j++)
				result[i][j] = lhs[i][j] - rhs[i][j];
		return result;
	}

	// 矩阵输出流
	template<int nrows, int ncols>
	std::ostream& operator<<(std::ostream& out, const Matrix<nrows, ncols>& m) {
		for (int i = 0; i < nrows; i++)
			out << m[i] << std::endl;
		return out;
	}

	// 模板元编程：递归计算行列式
	template<int n>
	struct dt {
		static double det(const Matrix<n, n>& src) {
			double ret = 0;
			for (int i = 0; i < n; i++)
				ret += src[0][i] * src.cofactor(0, i);
			return ret;
		}
	};

	// 模板特化：递归终止条件（1x1矩阵）
	template<>
	struct dt<1> {
		static double det(const Matrix<1, 1>& src) {
			return src[0][0];
		}
	};

#pragma endregion
}








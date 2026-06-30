#pragma once
#include <Common.h>

class TransformUtils
{
public:
	// 行列からヨー、ピッチ、ロールを行列を計算する。
	static bool MatrixToRollPitchYaw(const DirectX::XMFLOAT4X4& m, float& pitch, float& yaw, float& roll);

	// クォータニオンからヨー、ピッチ、ロールを行列を計算する。
	static bool QuaternionToRollPitchYaw(const Quaternion& q, float& pitch, float& yaw, float& roll);
};
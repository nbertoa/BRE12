#include "MathUtils.h"

#include <cfloat>
#include <cmath>

using namespace DirectX;

const float MathUtils::Infinity{ FLT_MAX };
const float MathUtils::Pi{ 3.1415926535f };

void MathUtils::ComputeMatrix(
	DirectX::XMFLOAT4X4& m,
	const float tx,
	const float ty,
	const float tz,
	const float sx,
	const float sy,
	const float sz,
	const float rx,
	const float ry,
	const float rz) noexcept
{
	DirectX::XMStoreFloat4x4(&m, 
		DirectX::XMMatrixScaling(sx, sy, sz) *  
		DirectX::XMMatrixRotationX(rx) * 
		DirectX::XMMatrixRotationY(ry) * 
		DirectX::XMMatrixRotationZ(rz) *
		DirectX::XMMatrixTranslation(tx, ty, tz));
}
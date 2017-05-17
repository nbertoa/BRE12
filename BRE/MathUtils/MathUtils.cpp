#include "MathUtils.h"

#include <cfloat>
#include <cmath>

using namespace DirectX;

namespace BRE {
const float MathUtils::Infinity{ FLT_MAX };
const float MathUtils::Pi{ 3.1415926535f };

void
MathUtils::ComputeMatrix(XMFLOAT4X4& m,
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
    XMStoreFloat4x4(&m,
                    XMMatrixScaling(sx, sy, sz) *
                    XMMatrixRotationX(rx) *
                    XMMatrixRotationY(ry) *
                    XMMatrixRotationZ(rz) *
                    XMMatrixTranslation(tx, ty, tz));
}
}
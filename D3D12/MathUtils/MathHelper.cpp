#include "MathHelper.h"

#include <cfloat>
#include <cmath>

using namespace DirectX;

const float MathHelper::Infinity{ FLT_MAX };
const float MathHelper::Pi{ 3.1415926535f };

float MathHelper::AngleFromXY(const float x, const float y) {
	float theta{ 0.0f };
 
	// Quadrant I or IV
	if (x >= 0.0f) {
		// If x = 0, then atanf(y/x) = +pi/2 if y > 0
		//                atanf(y/x) = -pi/2 if y < 0
		theta = atanf(y / x); // in [-pi/2, +pi/2]

		if (theta < 0.0f) {
			theta += 2.0f * Pi; // in [0, 2*pi).
		}
	}
	else {
		// Quadrant II or III
		theta = atanf(y / x) + Pi; // in [0, 2*pi).
	}

	return theta;
}

XMVECTOR MathHelper::RandUnitVec3() {
	const XMVECTOR One(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
	const XMVECTOR Zero(XMVectorZero());

	// Keep trying until we get a point on/in the hemisphere.
	while(true) {
		// Generate random point in the cube [-1,1]^3.
		const XMVECTOR v(XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f));

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.

		if (XMVector3Greater(XMVector3LengthSq(v), One)) {
			continue;
		}

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n) {
	const XMVECTOR One(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
	const XMVECTOR Zero(XMVectorZero());

	// Keep trying until we get a point on/in the hemisphere.
	while(true) {
		// Generate random point in the cube [-1,1]^3.
		const XMVECTOR v(XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f));

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.
		
		if (XMVector3Greater(XMVector3LengthSq(v), One)) {
			continue;
		}

		// Ignore points in the bottom hemisphere.
		if (XMVector3Less(XMVector3Dot(n, v), Zero)) {
			continue;
		}

		return XMVector3Normalize(v);
	}
}
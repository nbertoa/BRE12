#include "CBuffers.h"

const FrameCBuffer& FrameCBuffer::operator=(const FrameCBuffer& instance) {
	if (this == &instance) {
		return *this;
	}
	
	mViewMatrix = instance.mViewMatrix;
	mInverseViewMatrix = instance.mInverseViewMatrix;
	mProjectionMatrix = instance.mProjectionMatrix;
	mInverseProjectionMatrix = instance.mInverseProjectionMatrix;
	mEyePosW = instance.mEyePosW;

	return *this;
}
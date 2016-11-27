#include "CBuffers.h"

const FrameCBuffer& FrameCBuffer::operator=(const FrameCBuffer& instance) {
	if (this == &instance) {
		return *this;
	}
	
	mView = instance.mView;
	mInvView = instance.mInvView;
	mProj = instance.mProj;
	mInvProj = instance.mInvProj;
	mEyePosW = instance.mEyePosW;

	return *this;
}
#include "UploadBufferManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
tbb::concurrent_unordered_set<UploadBuffer*> UploadBufferManager::mUploadBuffers;
std::mutex UploadBufferManager::mMutex;

void
UploadBufferManager::Clear() noexcept
{
    for (UploadBuffer* uploadBuffer : mUploadBuffers) {
        BRE_ASSERT(uploadBuffer != nullptr);
        delete uploadBuffer;
    }

    mUploadBuffers.clear();
}

UploadBuffer&
UploadBufferManager::CreateUploadBuffer(const std::size_t elementSize,
                                        const std::uint32_t elementCount) noexcept
{
    BRE_ASSERT(elementSize > 0UL);
    BRE_ASSERT(elementCount > 0U);

    UploadBuffer* uploadBuffer = new UploadBuffer(DirectXManager::GetDevice(),
                                                  elementSize,
                                                  elementCount);
    mUploadBuffers.insert(uploadBuffer);

    return *uploadBuffer;
}

}


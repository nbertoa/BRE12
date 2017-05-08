#include "UploadBufferManager.h"

#include <DirectXManager/DirectXManager.h>
#include <Utils/DebugUtils.h>

namespace BRE {
UploadBufferManager::UploadBuffers UploadBufferManager::mUploadBuffers;
std::mutex UploadBufferManager::mMutex;

void
UploadBufferManager::EraseAll() noexcept
{
    for (UploadBuffer* uploadBuffer : mUploadBuffers) {
        BRE_ASSERT(uploadBuffer != nullptr);
        delete uploadBuffer;
    }
}

UploadBuffer&
UploadBufferManager::CreateUploadBuffer(const std::size_t elementSize,
                                        const std::uint32_t elementCount) noexcept
{
    BRE_ASSERT(elementSize > 0UL);
    BRE_ASSERT(elementCount > 0U);

    UploadBuffer* uploadBuffer = new UploadBuffer(DirectXManager::GetDevice(), elementSize, elementCount);
    mUploadBuffers.insert(uploadBuffer);

    return *uploadBuffer;
}

}


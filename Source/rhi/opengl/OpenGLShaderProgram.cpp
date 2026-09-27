#include "OpenGLShaderProgram.h"
#include "rhi/include/RHIDevice.h"

namespace engine {
namespace rhi {

OpenGLShaderProgram::OpenGLShaderProgram(ProgramHandle programHandle, GLuint glProgramId, RHIDevice& device)
    : mProgramHandle(programHandle)
    , mGLProgramId(glProgramId)
    , mDevice(&device) {
}

OpenGLShaderProgram::~OpenGLShaderProgram() {
    if (mDevice && static_cast<bool>(mProgramHandle)) {
        mDevice->destroyProgram(mProgramHandle);
    }
}

OpenGLShaderProgram::OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept
    : mProgramHandle(other.mProgramHandle)
    , mGLProgramId(other.mGLProgramId)
    , mDevice(other.mDevice)
    , mStatus(other.mStatus) {
    other.mProgramHandle.clear();
    other.mGLProgramId = 0;
    other.mDevice = nullptr;
}

OpenGLShaderProgram& OpenGLShaderProgram::operator=(OpenGLShaderProgram&& other) noexcept {
    if (this != &other) {
        // 先销毁自己持有的资源
        if (mDevice && static_cast<bool>(mProgramHandle)) {
            mDevice->destroyProgram(mProgramHandle);
        }
        mProgramHandle = other.mProgramHandle;
        mGLProgramId = other.mGLProgramId;
        mDevice = other.mDevice;
        mStatus = other.mStatus;
        other.mProgramHandle.clear();
        other.mGLProgramId = 0;
        other.mDevice = nullptr;
    }
    return *this;
}

// ============================================================================
// Program 激活
// ============================================================================

bool OpenGLShaderProgram::ensureReady() {
    if (mStatus == ProgramStatus::Pending && mDevice) {
        mStatus = mDevice->waitProgram(mProgramHandle) ? ProgramStatus::Ready : ProgramStatus::Failed;
    }
    return mStatus == ProgramStatus::Ready;
}

void OpenGLShaderProgram::use() {
    if (!ensureReady()) return;
    glUseProgram(mGLProgramId);
}

void OpenGLShaderProgram::unuse() {
    glUseProgram(0);
}

} // namespace rhi
} // namespace engine

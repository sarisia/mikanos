#pragma once

#include <array>

#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

class Error {
public:
    enum Code {
        kSuccess,
        kFull,
        kEmpty,
        kNoEnoughMemory,
        kIndexOutOfRange,
        kHostControllerNotHalted,
        kInvalidSlotID,
        kPortNotConnected,
        kInvalidEndpointNumber,
        kTransferRingNotSet,
        kAlreadyAllocated,
        kNotImplemented,
        kInvalidDescriptor,
        kBufferTooSmall,
        kUnknownDevice,
        kNoCorrespondingSetupStage,
        kTransferFailed,
        kInvalidPhase,
        kUnknownXHCISpeedID,
        kNoWaiter,
        kNoPCIMSI,
        kUnknownPixelFormat,
        kNoSuchTask,
        kLastOfCode,
    };

    Error(Code code, const char* file, int line)
        :code_(code), file_(file), line_(line) {}

    operator bool() const {
        return this->code_ != kSuccess;
    }

    const char* Name() const {
        return code_names_[static_cast<int>(this->code_)];
    }

    Code Cause() const {
        return this->code_;
    }

    const char* File() const {
        return this->file_;
    }

    int Line() const {
        return this->line_;
    }

private:
    Code code_;
    const char* file_;
    int line_;
    
    static constexpr std::array<const char*, kLastOfCode> code_names_ = {
        "kSuccess",
        "kFull",
        "kEmpty",
        "kNoEnoughMemory",
        "kIndexOutOfRange",
        "kHostControllerNotHalted",
        "kInvalidSlotID",
        "kPortNotConnected",
        "kInvalidEndpointNumber",
        "kTransferRingNotSet",
        "kAlreadyAllocated",
        "kNotImplemented",
        "kInvalidDescriptor",
        "kBufferTooSmall",
        "kUnknownDevice",
        "kNoCorrespondingSetupStage",
        "kTransferFailed",
        "kInvalidPhase",
        "kUnknownXHCISpeedID",
        "kNoWaiter",
        "kNoPCIMSI",
        "kUnknownPixelFormat",
        "kNoSuchTask",
    };
    static_assert(kLastOfCode == code_names_.size());
};

template <class T>
struct WithError {
    T value;
    Error error;
};

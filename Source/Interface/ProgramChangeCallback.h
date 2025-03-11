#pragma once

class ProgramChangeCallback {
public:
    virtual ~ProgramChangeCallback() = default;

    virtual void onProgramChange(int programNumber) = 0;
};

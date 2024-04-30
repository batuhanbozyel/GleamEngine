#pragma once
#include "Core/Subsystem.h"

namespace Gleam {

class BinarySerializer final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
private:
    
};

} // namespace Gleam

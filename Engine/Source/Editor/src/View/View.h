//
//  ViewStack.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once

namespace GEditor {

class View
{
public:
    
    virtual ~View() = default;
    
    virtual void Render() {}
    
};

} // namespace GEditor

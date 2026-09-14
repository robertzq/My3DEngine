#include "Engine/UI/MenuInput.h"
#include "Engine/UI/Menu.h"
#include "Engine/Input.h"

namespace MenuInput {

void Handle(Menu& menu) {
    if (!menu.IsOpen()) return;

    if (Input::Pressed("UIUp")) menu.FocusPrev();
    if (Input::Pressed("UIDown")) menu.FocusNext();

    // UILeft/UIRight 留给后续 Step 的交互组件（Toggle/Slider）处理。

    if (Input::Pressed("UIConfirm")) menu.Confirm();
    if (Input::Pressed("UICancel")) menu.Cancel();
}

}

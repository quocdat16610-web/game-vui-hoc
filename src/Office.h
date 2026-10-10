// =============================================================================
//  Office.h — văn phòng 3D của anh dev Minh: nơi đi lại, mở máy tính để học,
//  bước lên võ đài để chơi thử game đang làm.
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>

namespace office {
    enum class Action { None, OpenComputer, EnterArena };

    void   Init();
    void   Shutdown();
    void   RenderScreens();                 // vẽ màn hình máy tính (render texture) — gọi TRƯỚC BeginDrawing
    Action Update(float dt, bool titleMode);
    void   Draw(bool titleMode);
    void   Say(const std::string& msg, float seconds = 6.0f);   // lời thoại của anh dev
    bool   ModalOpen();
}

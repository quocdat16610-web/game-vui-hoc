// =============================================================================
//  Office.h — văn phòng 3D của anh dev Minh: nơi đi lại, mở máy tính để học,
//  bước lên võ đài để chơi thử game đang làm.
// =============================================================================
#pragma once
#include "raylib.h"
#include <string>

namespace office {
    enum class Action { None, OpenComputer, EnterArena, ExitToWorld };

    void   Init();
    void   Shutdown();
    void   RenderOffscreen();               // màn hình máy tính + bóng đổ — gọi TRƯỚC khi vẽ cảnh
    Action Update(float dt, bool titleMode);
    void   Draw3D();
    void   DrawHUD(bool titleMode);
    void   Say(const std::string& msg, float seconds = 6.0f);   // lời thoại của anh dev
    bool   ModalOpen();
    void   SpawnAtDoor();                     // đặt anh dev ở cửa ra vào (khi bước vào từ ngoài đảo)
}

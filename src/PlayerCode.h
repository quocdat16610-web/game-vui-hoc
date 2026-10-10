// =============================================================================
//  PlayerCode.h — GAME CHẠY CODE CỦA NGƯỜI CHƠI.
//  Mỗi bài đã qua thử thách: hàm người chơi viết được nạp vào trình thông dịch
//  và game gọi nó thay cho code có sẵn. Nếu code lỗi khi chạy, game báo lỗi
//  và tạm dùng code mẫu để không bị dừng giữa trận.
// =============================================================================
#pragma once
#include <string>
#include <vector>

namespace pcode {
    void LoadAll();                                       // khởi động: đọc code/bai_XX.cpp
    std::string LoadSource(int lesson);                   // code đã lưu (hoặc code khởi đầu)
    void SaveSource(int lesson, const std::string& src);
    bool HasSaved(int lesson);
    void Activate(int lesson, const std::string& src);    // sau khi qua thử thách
    bool Active(int lesson);
    int  ActiveCount();
    std::string TakeError();                              // lỗi khi chạy (để hiện thông báo)

    // ---- Các hàm game gọi. fb = giá trị dự phòng khi chưa có code người chơi ----
    float TiLeMau(int mau, int mauToiDa, float fb);                         // Bài 1
    bool  KetThucHiep(int mau1, int mau2, bool fb);                         // Bài 2
    int   SoDonDeKO(int mau, int satThuong, int fb);                        // Bài 2
    int   TinhSatThuong(int lucDanh, bool biDo, int fb);                    // Bài 3
    bool  NhanDon(int& mau, bool& conSong, int satThuong);                  // Bài 4 (false = chưa có)
    bool  VoSiCuaBan(float& tocDo, int& lucDam, int& lucDa, int& mau);      // Bài 5
    bool  CapNhatHat(float& x, float& y, float& vx, float& vy, float& life, float dt);   // Bài 6
    int   DonManhNhat(const std::vector<int>& cacDon, int fb);              // Bài 7
    float DiChuyen(float x, int huong, float tocDo, float dt, float fb);    // Bài 8
    bool  TrongLuc(float& y, float& vy, float dt);                          // Bài 9
    bool  DangGayDon(int loaiDon, float t, bool fb);                        // Bài 10
    bool  VaCham(float aMinX, float aMinY, float aMaxX, float aMaxY,
                 float bMinX, float bMinY, float bMaxX, float bMaxY, bool fb);   // Bài 11
    int   ChonHanhDong(float khoangCach, bool doiThuDangDanh, float ngauNhien, int fb);  // Bài 12
    float Lerp(float a, float b, float t, float fb);                        // Bài 13
    int   NguoiThang(float tiLe1, float tiLe2, int fb);                     // Bài 14
    bool  MauAmThanh(std::vector<float>& out, int sampleRate, float seconds);           // Bài 16
}

# CODE VÕ ĐÀI 🥋 — học lập trình game C++ bằng cách... chơi game

**Code Võ Đài** là một game 3D chạy trực tiếp trên máy (file `.exe`, không cần trình duyệt), viết bằng **C++17 + [raylib](https://www.raylib.com)**.

Bạn vào vai **Minh**, một dev indie đang viết một **game đối kháng 3D** bằng C++.

## 🏝️ Đảo Lập Trình — thế giới mở do CHÍNH BẠN xây

Vào game, bạn đứng trên một hòn đảo lục giác **gần như trống trơn**: chỉ có đất, nước và Studio của Minh. **Mọi thứ còn lại do bạn tự tay xây** — game chỉ cung cấp kiến thức, không có code giải sẵn:

- ⌨️ **Phím `C` — Bản thiết kế**: tự viết code C++ trong hàm `void XayDung()` rồi bấm `F5`. Game "cắm" sẵn các lệnh như `Dat(q, r, NHA)`, `Xoa(q, r)`, `OTrong(q, r)`, `KhoangCach(...)`, `LangGiengQ/R(h)`, `NgauNhien(a, b)`... Muốn trồng 100 cây? Tự viết vòng `for`. Muốn cả khu phố? Hai vòng `for` lồng nhau. Bên cạnh có **8 thẻ Kiến thức** giải thích cú pháp (gọi hàm, biến, for, if, hàm tự viết, vòng lặp lồng nhau, continue/break, lưới lục giác) bằng ví dụ không liên quan — phần xây là của bạn. Code lưu ở `code/ban_thiet_ke.cpp`.
- 🖱️ **Phím `B` — Chế độ xây**: đặt / xóa / xoay từng vật thể bằng chuột (Ctrl+Z để hoàn tác).
- 🏠 30+ vật thể của bộ KayKit Medieval Hexagon: nhà, chợ, giếng, lò rèn, cối xay, tháp, cây, đá, núi, đèn đường... **Đường đi tự nối** (auto-tiling bằng bitmask). Một số công trình mở khóa theo bài học.
- 🏟️ **Đấu trường** và 📖 **Thư viện** bạn xây xong thì đi vào được (chơi game đối kháng / học bài). Mỗi ngôi nhà có **dân làng** đến ở, đi dạo và đố bạn câu hỏi về bài đã học.
- 🌗 Chu kỳ **ngày / đêm**, đèn đường tự sáng khi trời tối, bóng đổ, mây trôi, bản đồ nhỏ, 8 nhiệm vụ gợi ý (có xu thưởng). Thế giới tự lưu vào `world.txt`.

Đi tới cửa Studio (tòa nhà đỏ) và nhấn `E` để vào văn phòng 3D của Minh:

- 💻 **Máy tính**: học 16 bài từ cơ bản đến nâng cao (lý thuyết, code mẫu tô màu, câu hỏi trắc nghiệm).
- 🥊 **Võ đài**: chơi thử game đối kháng mà Minh đang làm. **Mỗi bài học xong sẽ mở khóa đúng một tính năng** của game: học "hàm" thì có đòn đấm, học "AABB" thì có hitbox, học "AI" thì bao cát biến thành đối thủ biết đánh trả...
- 📚 **Kệ sách**: xem lộ trình dự án.
- ☕ **Máy cà phê**: nhận mẹo lập trình C++.

**Đồ họa**: ánh sáng bằng shader GLSL (Lambert + Blinn-Phong + đèn điểm), bóng đổ thời gian thực (shadow map), hậu kỳ bloom / FXAA / vignette, slow-motion và tách màu khi K.O., pháo giấy, màn hình LED, sân khấu Neon ban đêm. **Âm thanh**: mọi tiếng đấm, đá, đỡ và nhạc nền chiptune đều được tổng hợp bằng code, không dùng file âm thanh nào.

## Bạn TỰ CODE từng tính năng

Mỗi bài học kết thúc bằng một **thử thách code**: bạn gõ C++ (bài 15 là GLSL) ngay trong trình soạn thảo của game, bấm **F5** để chấm. Game có sẵn một **trình thông dịch C++ thu nhỏ** tự viết (`src/MiniCpp.cpp`) nên không cần cài trình biên dịch: báo lỗi theo dòng bằng tiếng Việt (thiếu `;`, chia cho 0, vòng lặp vô hạn, biến chưa khai báo...) và chạy các test tự động.

Qua thử thách, **game dùng CHÍNH HÀM BẠN VIẾT**:

| Bài | Bạn viết | Game dùng để |
|---|---|---|
| 1 | `float TiLeMau(int mau, int mauToiDa)` | vẽ thanh máu |
| 2 | `bool KetThucHiep(...)`, `int SoDonDeKO(...)` (vòng lặp) | kết thúc hiệp, hiện số đòn còn lại |
| 3 | `int TinhSatThuong(int lucDanh, bool biDo)` | tính sát thương mỗi đòn |
| 4 | `class Fighter { void NhanDon(int) }` | trừ máu, phát hiện K.O. |
| 5 | `class VoSiCuaBan : public Character` | võ sĩ "Của bạn" do bạn thiết kế chỉ số |
| 6 | `void CapNhatHat(Particle& p, float dt)` | chuyển động tia lửa |
| 7 | `int DonManhNhat(const std::vector<int>&)` | thống kê "đòn mạnh nhất" |
| 8 | `float DiChuyen(x, huong, tocDo, dt)` | bước chân võ sĩ (delta time) |
| 9 | `void TrongLuc(float& y, float& vy, float dt)` | vật lý cú nhảy |
| 10 | `bool DangGayDon(int loaiDon, float t)` | frame data: khi nào hitbox bật |
| 11 | `bool VaCham(Hop a, Hop b)` | hitbox / hurtbox |
| 12 | `int ChonHanhDong(...)` | bộ não AI đối thủ |
| 13 | `float Lerp(a, b, t)` | camera bám theo trận đấu |
| 14 | `int NguoiThang(tiLe1, tiLe2)` | luật hết giờ |
| 15 | GLSL `float AnhSang(vec3 N, vec3 L)` | ánh sáng mặt trời trên GPU |
| 16 | `float MauAmThanh(float t)` | âm thanh cú đấm |

Code được lưu ở thư mục `code/` cạnh file exe (`code/bai_01.cpp`...), mở được bằng IDE thật. Nếu code của bạn lỗi khi đang chơi, game báo lỗi và tạm dùng code mẫu. Bí quá? Có nút **Gợi ý**, và sau 2 lần sai thì mở được **Lời giải** (nhưng bài đó chỉ được 1 sao).

Mọi khái niệm trong bài học đều có **file ví dụ chạy được** trong thư mục [`lessons/`](lessons) và đều **được dùng thật** trong mã nguồn game ở [`src/`](src). Học xong, bạn đọc được toàn bộ code của chính game mình vừa chơi.

---

## 1. Chạy game ngay (Windows)

1. Tải file [`release/CodeVoDai-Windows.zip`](release/CodeVoDai-Windows.zip) (nút **Download raw file** trên GitHub).
2. Giải nén ra một thư mục.
3. Bấm đúp `CodeVoDai.exe`.

> Phải giữ thư mục `assets/` nằm cạnh `CodeVoDai.exe` (chứa font tiếng Việt, model 3D, shader). Tiến độ học được lưu vào file `save.txt` cùng thư mục.
>
> Windows SmartScreen có thể cảnh báo vì file exe chưa được ký số: chọn **More info → Run anyway**.

Mỗi lần push code, GitHub Actions cũng build lại bản Windows bằng Visual Studio. Bạn tải ở tab **Actions** → lần chạy mới nhất → mục **Artifacts** → `CodeVoDai-Windows` (gồm cả file `.exe` của từng bài học).

## 2. Điều khiển

| Ở đâu | Phím |
|---|---|
| Đảo Lập Trình | `W A S D`: đi · `Shift`: chạy · `Space`: nhảy · chuột phải kéo / `Q`: xoay camera · lăn chuột: zoom · `E`: tương tác · `B`: xây bằng chuột · `C`: xây bằng code |
| Chế độ xây (`B`) | chuột trái: đặt · chuột phải / `X`: xóa · `R`: xoay · `Tab` / `,` `.`: đổi vật thể · `Ctrl+Z`: hoàn tác · `Q E`: xoay camera |
| Bản thiết kế (`C`) | `F5`: chạy `XayDung()` · `Esc`: rời ô code · `C`/`Esc`: đóng |
| Văn phòng | `W A S D` / phím mũi tên: đi lại · `E`: tương tác · thảm đỏ ở cửa: ra đảo |
| Máy tính (bài học) | `→` / `Enter`: tiếp · `←`: quay lại · `1-4`: chọn đáp án · `R`: thử lại · con lăn chuột: cuộn · `Esc`: thoát |
| Thử thách code | `F5` hoặc `Ctrl+Enter`: chạy test · `Ctrl+Z/Y`: hoàn tác/làm lại · `Ctrl+C/X/V/A` · `Ctrl+/`: chú thích · `Tab`/`Shift+Tab`: thụt lề · `Esc`: rời ô code |
| Võ đài | `A D`: đi · `W`/`Space`: nhảy · `J`: đấm · `K`: đá · `L` (giữ): đỡ · `Shift`: lướt · `H`: xem hitbox · `P`: tạm dừng · `Esc`: về văn phòng |
| Màn chọn võ sĩ | `A D`: chọn · `W S`: độ khó · `Q E`: đổi sân khấu (sau Bài 15) · `Enter`: vào trận |
| Mọi lúc | `F2`: chất lượng đồ họa (Thấp / Trung bình / Cao) · `M`: bật/tắt âm thanh · `F11`: toàn màn hình |

Phím chỉ hoạt động khi bạn đã học bài tương ứng. Bấm phím của tính năng chưa học, game sẽ nhắc bạn cần học bài nào.

Dành cho giáo viên / muốn xem trước: trong văn phòng nhấn `F10` để mở khóa toàn bộ. Có thể chạy kèm tham số: `CodeVoDai.exe --unlock-all`, `--reset` (xóa tiến độ), `--lessons=5` (coi như đã học 5 bài), `--hour=21` (đặt giờ trên đảo).

## 3. Lộ trình 16 bài

| # | Cấp độ | Bài học | Mở khóa trong game đối kháng | File ví dụ |
|---|---|---|---|---|
| 1 | Cơ bản | Biến & kiểu dữ liệu | Thanh máu | [`01_bien_va_kieu.cpp`](lessons/01_bien_va_kieu.cpp) |
| 2 | Cơ bản | Rẽ nhánh & vòng lặp (game loop) | Võ đài chạy được | [`02_re_nhanh_vong_lap.cpp`](lessons/02_re_nhanh_vong_lap.cpp) |
| 3 | Cơ bản | Hàm, tham trị & tham chiếu | Đòn đấm | [`03_ham.cpp`](lessons/03_ham.cpp) |
| 4 | Cơ bản | Struct & class (đóng gói) | Đá + đỡ đòn | [`04_class.cpp`](lessons/04_class.cpp) |
| 5 | Cơ bản | Kế thừa & đa hình | Chọn nhân vật | [`05_ke_thua_da_hinh.cpp`](lessons/05_ke_thua_da_hinh.cpp) |
| 6 | Trung cấp | Con trỏ, bộ nhớ, smart pointer | Hiệu ứng tia lửa | [`06_con_tro_bo_nho.cpp`](lessons/06_con_tro_bo_nho.cpp) |
| 7 | Trung cấp | STL: vector, map, algorithm, lambda | Combo + thống kê trận | [`07_stl.cpp`](lessons/07_stl.cpp) |
| 8 | Trung cấp | Game loop chuẩn & delta time | Lướt (dash) | [`08_delta_time.cpp`](lessons/08_delta_time.cpp) |
| 9 | Trung cấp | Toán vector 3D & trọng lực | Nhảy + tự quay mặt | [`09_vector_3d.cpp`](lessons/09_vector_3d.cpp) |
| 10 | Trung cấp | Input & máy trạng thái (FSM) | Hoạt ảnh + choáng | [`10_may_trang_thai.cpp`](lessons/10_may_trang_thai.cpp) |
| 11 | Nâng cao | Va chạm AABB: hitbox/hurtbox | Hitbox chính xác, đẩy nhau | [`11_va_cham_aabb.cpp`](lessons/11_va_cham_aabb.cpp) |
| 12 | Nâng cao | AI đối thủ | Đối thủ biết đánh trả | [`12_ai_doi_thu.cpp`](lessons/12_ai_doi_thu.cpp) |
| 13 | Nâng cao | raylib: camera 3D & game feel | Camera động, rung, hit-stop | [`13_camera_raylib.cpp`](lessons/13_camera_raylib.cpp) |
| 14 | Nâng cao | Kiến trúc game & tối ưu (Scene, ECS) | Hiệp đấu 2/3, đồng hồ, độ khó | [`14_kien_truc.cpp`](lessons/14_kien_truc.cpp) |
| 15 | Nâng cao | Shader & ánh sáng (GLSL), bóng đổ, hậu kỳ | Sân khấu Neon ban đêm | [`15_shader_anh_sang.cpp`](lessons/15_shader_anh_sang.cpp) |
| 16 | Nâng cao | Âm thanh: tổng hợp âm thanh bằng code | Nhạc nền chiptune | [`16_am_thanh.cpp`](lessons/16_am_thanh.cpp) |

Mỗi bài có phần **bài tập tự làm**: sửa file ví dụ để thêm tính năng mới.

Máy yếu hoặc card đồ họa cũ: nhấn `F2` để giảm chất lượng đồ họa (lựa chọn được lưu lại). Game cần card hỗ trợ OpenGL 3.3.

## 4. Tự biên dịch (khuyến khích, vì bạn đang học code!)

Cần: một trình biên dịch C++17 và **CMake 3.16+**. Lần build đầu CMake sẽ tự tải raylib từ GitHub (cần mạng).

### Windows: Visual Studio 2022 (dễ nhất)

1. Cài [Visual Studio 2022 Community](https://visualstudio.microsoft.com/), chọn workload **Desktop development with C++** (đã có sẵn CMake).
2. **File → Open → Folder...** chọn thư mục dự án này. Visual Studio tự nhận `CMakeLists.txt`.
3. Chọn target `CodeVoDai.exe` ở thanh trên cùng, bấm ▶ để chạy. Các bài học là target `bai_01_bien_va_kieu.exe`, `bai_02_...`

Hoặc dùng dòng lệnh (Developer PowerShell):

```bash
cmake -S . -B build
cmake --build build --config Release
build\Release\CodeVoDai.exe
build\lessons\Release\bai_01_bien_va_kieu.exe
```

### Windows: MinGW / w64devkit, Linux, macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/CodeVoDai
./build/lessons/bai_03_ham
```

Linux cần thêm thư viện X11/OpenGL: `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.

Build file `.exe` cho Windows từ Linux: `cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release` (cần gói `mingw-w64`).

### Chạy nhanh một bài học console (không cần CMake)

Bài 1 đến 12 và bài 14 chỉ dùng thư viện chuẩn C++ (bài 13, 15, 16 cần raylib nên hãy build bằng CMake):

```bash
g++ -std=c++17 lessons/01_bien_va_kieu.cpp -o bai01
./bai01
```

## 5. Đọc mã nguồn game

Sau khi học xong, hãy đọc `src/` theo thứ tự này:

| File | Nội dung | Liên quan bài |
|---|---|---|
| [`src/main.cpp`](src/main.cpp) | Game loop chính, chuyển màn hình | 2, 8, 14 |
| [`src/Common.h`](src/Common.h) / [`.cpp`](src/Common.cpp) | Danh sách tính năng, lưu/đọc tiến độ ra file | 1, 4 |
| [`src/Arena.h`](src/Arena.h) / [`.cpp`](src/Arena.cpp) | **Game đối kháng**: Fighter, FSM, vật lý, hitbox, AI, camera, hạt, sân khấu | 3 → 15 |
| [`src/FighterModel.cpp`](src/FighterModel.cpp) | Dựng võ sĩ từ khối hình, hoạt ảnh khớp tay/chân | 9, 10 |
| [`src/Gfx.cpp`](src/Gfx.cpp) + [`assets/shaders/`](assets/shaders) | Shader ánh sáng, bóng đổ, bloom, FXAA | 15 |
| [`src/Audio.cpp`](src/Audio.cpp) | Tổng hợp tiếng đấm/đá/đỡ và nhạc nền bằng toán | 16 |
| [`src/World.cpp`](src/World.cpp) | **Đảo Lập Trình**: lưới lục giác, sinh đảo bằng noise, gộp lưới, đường tự nối, API xây dựng cho code người chơi, ngày/đêm, dân làng | 2, 3, 6, 9, 13 |
| [`src/Character.cpp`](src/Character.cpp) | Nạp model glTF có xương, gắn mũ/áo vào xương, phát hoạt ảnh | 10 |
| [`src/Office.cpp`](src/Office.cpp) | Văn phòng 3D, di chuyển, va chạm với đồ vật, render texture | 9, 11, 13 |
| [`src/LessonScreen.cpp`](src/LessonScreen.cpp) | Màn hình học, quiz | 2, 7 |
| [`src/Lessons.cpp`](src/Lessons.cpp) | Toàn bộ nội dung bài học (dạng dữ liệu) | 14 |
| [`src/UI.cpp`](src/UI.cpp) | Font tiếng Việt, tự xuống dòng, tô màu cú pháp C++ | 1, 3, 7 |
| [`src/MiniCpp.cpp`](src/MiniCpp.cpp) | Trình thông dịch C++ thu nhỏ: lexer → parser → cây cú pháp → chạy | tất cả |
| [`src/Challenges.cpp`](src/Challenges.cpp) | Đề bài + test tự động của 16 thử thách | tất cả |
| [`src/PlayerCode.cpp`](src/PlayerCode.cpp) | Nối hàm người chơi viết vào game | tất cả |
| [`src/CodeEditor.cpp`](src/CodeEditor.cpp) | Trình soạn thảo code trong game | 2, 7 |

Trong `Arena.cpp`, mỗi dòng `if (Has(F_...))` là một "công tắc" tính năng. Thử tắt/bật để xem game thay đổi thế nào!

## 6. Cấu trúc thư mục

```
CMakeLists.txt        cấu hình build (tự tải raylib)
src/                  mã nguồn game
lessons/              16 file ví dụ cho 16 bài học
assets/fonts/         font hỗ trợ tiếng Việt (DejaVu Sans, JetBrains Mono)
assets/shaders/       shader GLSL: ánh sáng, bóng đổ, hậu kỳ (sửa thử được!)
assets/models/        model 3D KayKit (nhân vật có xương + bộ ô lục giác thời trung cổ)
tools/                script Python chuẩn bị model (tách hoạt ảnh, chép file)
cmake/                toolchain build .exe Windows từ Linux
release/              bản Windows dựng sẵn
.github/workflows/    CI: build Windows (Visual Studio) + Linux
```

## Giấy phép

- Font DejaVu: giấy phép Bitstream Vera (xem `assets/fonts/LICENSE-DejaVu.txt`).
- Font JetBrains Mono: SIL Open Font License 1.1 (xem `assets/fonts/LICENSE-JetBrainsMono.txt`).
- raylib: giấy phép zlib.
- Model 3D: [KayKit Character Pack: Adventurers](https://kaylousberg.itch.io/kaykit-adventurers) và [KayKit Medieval Hexagon Pack](https://kaylousberg.itch.io/kaykit-medieval-hexagon) của Kay Lousberg — CC0 (xem `assets/models/*/LICENSE-*.txt`).

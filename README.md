# CODE VÕ ĐÀI 🥋 — học lập trình game C++ bằng cách... chơi game

**Code Võ Đài** là một game 3D chạy trực tiếp trên máy (file `.exe`, không cần trình duyệt), viết bằng **C++17 + [raylib](https://www.raylib.com)**.

Bạn vào vai **Minh**, một dev indie đang viết một **game đối kháng 3D** bằng C++. Trong văn phòng 3D của Minh:

- 💻 **Máy tính**: học 14 bài từ cơ bản đến nâng cao (lý thuyết, code mẫu tô màu, câu hỏi trắc nghiệm).
- 🥊 **Võ đài**: chơi thử game đối kháng mà Minh đang làm. **Mỗi bài học xong sẽ mở khóa đúng một tính năng** của game: học "hàm" thì có đòn đấm, học "AABB" thì có hitbox, học "AI" thì bao cát biến thành đối thủ biết đánh trả...
- 📚 **Kệ sách**: xem lộ trình dự án.
- ☕ **Máy cà phê**: nhận mẹo lập trình C++.

Mọi khái niệm trong bài học đều có **file ví dụ chạy được** trong thư mục [`lessons/`](lessons) và đều **được dùng thật** trong mã nguồn game ở [`src/`](src). Học xong, bạn đọc được toàn bộ code của chính game mình vừa chơi.

---

## 1. Chạy game ngay (Windows)

1. Tải file [`release/CodeVoDai-Windows.zip`](release/CodeVoDai-Windows.zip) (nút **Download raw file** trên GitHub).
2. Giải nén ra một thư mục.
3. Bấm đúp `CodeVoDai.exe`.

> Phải giữ thư mục `assets/` nằm cạnh `CodeVoDai.exe` (chứa font tiếng Việt). Tiến độ học được lưu vào file `save.txt` cùng thư mục.
>
> Windows SmartScreen có thể cảnh báo vì file exe chưa được ký số: chọn **More info → Run anyway**.

Mỗi lần push code, GitHub Actions cũng build lại bản Windows bằng Visual Studio. Bạn tải ở tab **Actions** → lần chạy mới nhất → mục **Artifacts** → `CodeVoDai-Windows` (gồm cả file `.exe` của từng bài học).

## 2. Điều khiển

| Ở đâu | Phím |
|---|---|
| Văn phòng | `W A S D` / phím mũi tên: đi lại · `E`: tương tác |
| Máy tính (bài học) | `→` / `Enter`: tiếp · `←`: quay lại · `1-4`: chọn đáp án · `R`: thử lại · con lăn chuột: cuộn · `Esc`: thoát |
| Võ đài | `A D`: đi · `W`/`Space`: nhảy · `J`: đấm · `K`: đá · `L` (giữ): đỡ · `Shift`: lướt · `H`: xem hitbox · `P`: tạm dừng · `Esc`: về văn phòng |
| Mọi lúc | `F11`: toàn màn hình |

Phím chỉ hoạt động khi bạn đã học bài tương ứng. Bấm phím của tính năng chưa học, game sẽ nhắc bạn cần học bài nào.

Dành cho giáo viên / muốn xem trước: trong văn phòng nhấn `F10` để mở khóa toàn bộ. Có thể chạy kèm tham số: `CodeVoDai.exe --unlock-all`, `--reset` (xóa tiến độ), `--lessons=5` (coi như đã học 5 bài).

## 3. Lộ trình 14 bài

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

Mỗi bài có phần **bài tập tự làm**: sửa file ví dụ để thêm tính năng mới.

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

Bài 1 đến 12 và bài 14 chỉ dùng thư viện chuẩn C++:

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
| [`src/Arena.h`](src/Arena.h) / [`.cpp`](src/Arena.cpp) | **Game đối kháng**: Fighter, FSM, vật lý, hitbox, AI, camera, hạt | 3 → 14 |
| [`src/Office.cpp`](src/Office.cpp) | Văn phòng 3D, di chuyển, va chạm với đồ vật, render texture | 9, 11, 13 |
| [`src/LessonScreen.cpp`](src/LessonScreen.cpp) | Màn hình học, quiz | 2, 7 |
| [`src/Lessons.cpp`](src/Lessons.cpp) | Toàn bộ nội dung bài học (dạng dữ liệu) | 14 |
| [`src/UI.cpp`](src/UI.cpp) | Font tiếng Việt, tự xuống dòng, tô màu cú pháp C++ | 1, 3, 7 |

Trong `Arena.cpp`, mỗi dòng `if (Has(F_...))` là một "công tắc" tính năng. Thử tắt/bật để xem game thay đổi thế nào!

## 6. Cấu trúc thư mục

```
CMakeLists.txt        cấu hình build (tự tải raylib)
src/                  mã nguồn game
lessons/              14 file ví dụ cho 14 bài học
assets/fonts/         font hỗ trợ tiếng Việt (DejaVu Sans, JetBrains Mono)
cmake/                toolchain build .exe Windows từ Linux
release/              bản Windows dựng sẵn
.github/workflows/    CI: build Windows (Visual Studio) + Linux
```

## Giấy phép

- Font DejaVu: giấy phép Bitstream Vera (xem `assets/fonts/LICENSE-DejaVu.txt`).
- Font JetBrains Mono: SIL Open Font License 1.1 (xem `assets/fonts/LICENSE-JetBrainsMono.txt`).
- raylib: giấy phép zlib.

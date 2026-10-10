// =============================================================================
//  MiniCpp.h — trình thông dịch một TẬP CON của C++ (viết tay, không cần
//  trình biên dịch ngoài). Dùng để chạy code người chơi gõ trong game.
//
//  Hỗ trợ: int/float/double/bool/auto, const, tham chiếu &, struct/class
//  (biến + hàm thành viên, kế thừa đơn giản), enum, std::vector, vec3 (GLSL),
//  if/else, switch, while, do-while, for, range-for, break/continue/return,
//  toán tử số học/so sánh/logic, ++/--, gán kép, ép kiểu, toán tử ba ngôi,
//  std::cout (để in debug), các hàm toán học thông dụng.
//  Không hỗ trợ: con trỏ, template tự viết, chuỗi (ngoài cout), new/delete.
//
//  Cấu trúc kinh điển của một trình thông dịch:
//     mã nguồn --(Lexer)--> token --(Parser)--> cây cú pháp (AST)
//              --(Interpreter)--> chạy từng nút của cây
// =============================================================================
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mcpp {

enum class Kind { Void, Int, Float, Bool, Struct, Vector, Str, Cout, InitList };

struct StructDef;
struct TypeInfo {
    Kind kind = Kind::Void;
    const StructDef* sdef = nullptr;         // khi kind == Struct
    std::shared_ptr<TypeInfo> elem;          // khi kind == Vector
    bool isRef = false, isConst = false, isAuto = false;
    std::string Name() const;
};
using TypeP = std::shared_ptr<TypeInfo>;

struct Value;
using Slot = std::shared_ptr<Value>;         // một "ô nhớ"; tham chiếu = dùng chung ô nhớ

struct Value {
    Kind kind = Kind::Void;
    long long i = 0;                         // Int, Bool (0/1)
    double f = 0;                            // Float
    std::string s;                           // Str
    const StructDef* sdef = nullptr;
    std::vector<Slot> items;                 // trường của struct, hoặc phần tử vector
    TypeP elem;                              // kiểu phần tử của vector
    bool readonly = false;                   // biến const

    static Value Int(long long v);
    static Value Float(double v);
    static Value Bool(bool v);
    double AsFloat() const;
    long long AsInt() const;
    bool AsBool() const;
    bool IsNumber() const { return kind == Kind::Int || kind == Kind::Float || kind == Kind::Bool; }
    std::string ToString() const;
};

struct Error {
    int line = 0;
    std::string msg;
};

struct Expr;
struct Stmt;
using ExprP = std::shared_ptr<Expr>;
using StmtP = std::shared_ptr<Stmt>;

struct Param {
    TypeP type;
    std::string name;
    ExprP def;                               // giá trị mặc định (có thể rỗng)
};

struct FuncDef {
    std::string name;
    TypeP ret;
    std::vector<Param> params;
    StmtP body;
    const StructDef* owner = nullptr;        // hàm thành viên của struct nào
    int line = 0;
};

struct FieldDef {
    TypeP type;
    std::string name;
    ExprP init;
};

struct StructDef {
    std::string name, base;
    std::vector<FieldDef> fields;
    std::map<std::string, std::shared_ptr<FuncDef>> methods;
    int FieldIndex(const std::string& n) const;
};

class Program {
public:
    Program();
    ~Program();

    // Biên dịch (lexer + parser). Trả về false và điền errors nếu có lỗi cú pháp.
    bool Compile(const std::string& source, std::vector<Error>& errors);

    bool HasFunction(const std::string& name) const;
    const StructDef* FindStruct(const std::string& name) const;
    const std::map<std::string, std::shared_ptr<StructDef>>& Structs() const { return structs_; }

    // Gọi hàm. Tham số tham chiếu (&) sẽ sửa trực tiếp Slot truyền vào.
    // Ném Error nếu lỗi khi chạy (chia cho 0, lặp vô hạn...).
    Value Call(const std::string& name, const std::vector<Slot>& args);
    Value CallMethod(const Slot& object, const std::string& method, const std::vector<Slot>& args);

    Slot NewObject(const std::string& structName);         // tạo đối tượng với giá trị mặc định
    static Slot Field(const Slot& object, const std::string& field);
    static Slot MakeSlot(const Value& v) { return std::make_shared<Value>(v); }
    static Slot NewVector(Kind elemKind);

    std::string& Output() { return output_; }               // nội dung std::cout
    void SetStepLimit(long long n) { stepLimit_ = n; }

    struct Impl;
private:
    std::map<std::string, std::shared_ptr<StructDef>> structs_;
    std::unique_ptr<Impl> impl_;
    std::string output_;
    long long stepLimit_ = 400000;
};

} // namespace mcpp

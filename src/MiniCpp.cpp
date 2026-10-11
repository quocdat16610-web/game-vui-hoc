// =============================================================================
//  MiniCpp.cpp — trình thông dịch C++ thu nhỏ: Lexer -> Parser -> Interpreter
// =============================================================================
#include "MiniCpp.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace mcpp {

// =============================================================================
//  VALUE
// =============================================================================
Value Value::Int(long long v) { Value r; r.kind = Kind::Int; r.i = v; return r; }
Value Value::Float(double v) { Value r; r.kind = Kind::Float; r.f = v; return r; }
Value Value::Bool(bool v) { Value r; r.kind = Kind::Bool; r.i = v ? 1 : 0; return r; }

double Value::AsFloat() const { return kind == Kind::Float ? f : (double)i; }
long long Value::AsInt() const { return kind == Kind::Float ? (long long)f : i; }
bool Value::AsBool() const { return kind == Kind::Float ? f != 0.0 : i != 0; }

static std::string FormatFloat(double d) {
    if (std::isnan(d)) return "nan";
    if (std::isinf(d)) return d > 0 ? "inf" : "-inf";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.5g", d);
    std::string s = buf;
    if (s.find_first_of(".eE") == std::string::npos && s.find("nan") == std::string::npos) s += ".0";
    return s;
}

std::string Value::ToString() const {
    switch (kind) {
    case Kind::Int: return std::to_string(i);
    case Kind::Float: return FormatFloat(f);
    case Kind::Bool: return i ? "true" : "false";
    case Kind::Str: return s;
    case Kind::Struct: {
        std::string r = (sdef && sdef->name == "vec3") ? "(" : "{";
        for (size_t k = 0; k < items.size(); ++k) r += (k ? ", " : "") + items[k]->ToString();
        return r + ((sdef && sdef->name == "vec3") ? ")" : "}");
    }
    case Kind::Vector: {
        std::string r = "[";
        for (size_t k = 0; k < items.size(); ++k) r += (k ? ", " : "") + items[k]->ToString();
        return r + "]";
    }
    default: return "";
    }
}

std::string TypeInfo::Name() const {
    std::string n;
    switch (kind) {
    case Kind::Void: n = "void"; break;
    case Kind::Int: n = "int"; break;
    case Kind::Float: n = "float"; break;
    case Kind::Bool: n = "bool"; break;
    case Kind::Struct: n = sdef ? sdef->name : "struct"; break;
    case Kind::Vector: n = "std::vector<" + (elem ? elem->Name() : std::string("?")) + ">"; break;
    default: n = "?"; break;
    }
    return n;
}

int StructDef::FieldIndex(const std::string& n) const {
    for (size_t k = 0; k < fields.size(); ++k)
        if (fields[k].name == n) return (int)k;
    return -1;
}

static Error Err(int line, const std::string& msg) { return Error{ line, msg }; }

// =============================================================================
//  AST
// =============================================================================
enum class EK { Int, Float, Bool, Str, Var, Unary, Binary, Logical, Assign, IncDec, Ternary,
                Call, Method, Member, Index, Cast, InitList, Construct, This };

struct Expr {
    EK k;
    int line = 0;
    long long ival = 0;
    double fval = 0;
    std::string name, op;
    bool prefix = false;
    std::vector<ExprP> a;
    TypeP type;
};

enum class SK { Block, Decl, ExprS, If, While, DoWhile, For, RangeFor, Switch, Break, Continue, Return, Empty };

struct Case { ExprP label; size_t start; };
struct Decl { std::string name; ExprP init; int line; };

struct Stmt {
    SK k;
    int line = 0;
    std::vector<StmtP> body;
    ExprP e1, e2, e3;
    StmtP s1, s2, s3;
    TypeP type;
    std::vector<Decl> decls;
    std::string name;
    std::vector<Case> cases;
};

static ExprP MakeE(EK k, int line) { auto e = std::make_shared<Expr>(); e->k = k; e->line = line; return e; }
static StmtP MakeS(SK k, int line) { auto s = std::make_shared<Stmt>(); s->k = k; s->line = line; return s; }
static TypeP MakeT(Kind k) { auto t = std::make_shared<TypeInfo>(); t->kind = k; return t; }

// =============================================================================
//  LEXER
// =============================================================================
enum class TT { Ident, Int, Float, Str, Op, End };
struct Tok { TT t; std::string s; int line; long long iv = 0; double fv = 0; };

static std::vector<Tok> Lex(const std::string& src) {
    std::vector<Tok> out;
    size_t i = 0, n = src.size();
    int line = 1;
    static const char* ops[] = { "<<=", ">>=", "->", "::", "++", "--", "+=", "-=", "*=", "/=", "%=", "==", "!=", "<=", ">=",
                                 "&&", "||", "<<", ">>", "&=", "|=", "^=" };
    bool lineStart = true;
    while (i < n) {
        char c = src[i];
        if (c == '\n') { line++; i++; lineStart = true; continue; }
        if (c == ' ' || c == '\t' || c == '\r') { i++; continue; }
        if (c == '#' && lineStart) {                              // #include ... : bỏ qua
            while (i < n && src[i] != '\n') i++;
            continue;
        }
        lineStart = false;
        if (c == '/' && i + 1 < n && src[i + 1] == '/') { while (i < n && src[i] != '\n') i++; continue; }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) { if (src[i] == '\n') line++; i++; }
            if (i + 1 >= n) throw Err(line, "chú thích /* ... chưa được đóng bằng */");
            i += 2;
            continue;
        }
        if (isalpha((unsigned char)c) || c == '_') {
            size_t j = i;
            while (j < n && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;
            out.push_back({ TT::Ident, src.substr(i, j - i), line });
            i = j;
            continue;
        }
        if (isdigit((unsigned char)c) || (c == '.' && i + 1 < n && isdigit((unsigned char)src[i + 1]))) {
            size_t j = i;
            bool isFloat = false;
            while (j < n && (isdigit((unsigned char)src[j]) || src[j] == '.' || src[j] == '\'')) { if (src[j] == '.') isFloat = true; j++; }
            if (j < n && (src[j] == 'e' || src[j] == 'E')) {
                isFloat = true; j++;
                if (j < n && (src[j] == '+' || src[j] == '-')) j++;
                while (j < n && isdigit((unsigned char)src[j])) j++;
            }
            std::string num;
            for (size_t k = i; k < j; ++k) if (src[k] != '\'') num += src[k];
            if (j < n && (src[j] == 'f' || src[j] == 'F')) { isFloat = true; j++; }
            while (j < n && (src[j] == 'u' || src[j] == 'U' || src[j] == 'l' || src[j] == 'L')) j++;
            if (j < n && (isalpha((unsigned char)src[j]) || src[j] == '_'))
                throw Err(line, "số viết sai: '" + src.substr(i, j - i + 1) + "'");
            Tok t{ isFloat ? TT::Float : TT::Int, num, line };
            if (isFloat) t.fv = std::strtod(num.c_str(), nullptr);
            else t.iv = std::strtoll(num.c_str(), nullptr, 10);
            out.push_back(t);
            i = j;
            continue;
        }
        if (c == '"') {
            size_t j = i + 1;
            std::string s;
            while (j < n && src[j] != '"') {
                if (src[j] == '\n') throw Err(line, "chuỗi \"...\" chưa được đóng");
                if (src[j] == '\\' && j + 1 < n) {
                    char e = src[j + 1];
                    s += (e == 'n') ? '\n' : (e == 't' ? '\t' : e);
                    j += 2;
                    continue;
                }
                s += src[j++];
            }
            if (j >= n) throw Err(line, "chuỗi \"...\" chưa được đóng");
            out.push_back({ TT::Str, s, line });
            i = j + 1;
            continue;
        }
        if (c == '\'') {
            size_t j = i + 1;
            long long v = 0;
            if (j < n && src[j] == '\\' && j + 1 < n) { char e = src[j + 1]; v = (e == 'n') ? '\n' : e; j += 2; }
            else if (j < n) { v = (unsigned char)src[j]; j++; }
            if (j >= n || src[j] != '\'') throw Err(line, "ký tự '...' viết sai");
            Tok t{ TT::Int, "char", line };
            t.iv = v;
            out.push_back(t);
            i = j + 1;
            continue;
        }
        if ((unsigned char)c >= 0x80)
            throw Err(line, "có ký tự lạ (chữ có dấu?) nằm ngoài chú thích. Tên biến chỉ dùng a-z, A-Z, 0-9 và _");
        bool matched = false;
        for (const char* op : ops) {
            size_t L = std::char_traits<char>::length(op);
            if (src.compare(i, L, op) == 0) { out.push_back({ TT::Op, op, line }); i += L; matched = true; break; }
        }
        if (matched) continue;
        if (std::string("+-*/%=<>!&|^~?:;,.(){}[]").find(c) != std::string::npos) {
            out.push_back({ TT::Op, std::string(1, c), line });
            i++;
            continue;
        }
        throw Err(line, std::string("ký tự không hợp lệ: '") + c + "'");
    }
    out.push_back({ TT::End, "", line });
    return out;
}

// =============================================================================
//  INTERPRETER STATE (cần trước Parser để Parser ghi hàm/biến toàn cục vào)
// =============================================================================
enum class Flow { Normal, Break, Continue, Return };

struct Frame {
    std::vector<std::map<std::string, Slot>> scopes;
    Slot self;
    Value ret;
    const FuncDef* fn = nullptr;
};

struct Program::Impl {
    friend class Program;
    Program* P = nullptr;
    std::map<std::string, std::shared_ptr<FuncDef>> funcs;
    std::vector<StmtP> globalDecls;
    std::map<std::string, Slot> globals;
    std::map<std::string, long long> enumConsts;
    std::set<std::string> enumTypes;
    std::shared_ptr<StructDef> vec3;
    bool globalsReady = false;
    long long steps = 0;
    std::vector<Frame> frames;
    unsigned int seed = 1234567u;

    // --- tiện ích kiểu & giá trị ---
    Value Default(const TypeInfo& t);
    Value Copy(const Value& v);
    Value Convert(const Value& v, const TypeInfo& t, int line);
    void Store(const Slot& dst, const Value& v, int line);

    Value Eval(const ExprP& e);
    Slot EvalRef(const ExprP& e);
    Flow Exec(const StmtP& s);
    Value Invoke(const FuncDef& fn, const std::vector<Slot>& argSlots, const std::vector<ExprP>* argExprs,
                 const Slot& self, int line);
    Value CallBuiltin(const std::string& name, std::vector<Value>& args, int line, bool& found);
    Value VectorMethod(const Slot& obj, const std::string& m, std::vector<Value>& args, int line);
    Value BinaryOp(const std::string& op, const Value& a, const Value& b, int line);
    Slot Lookup(const std::string& name, int line);
    void EnsureGlobals();
    void Step(int line) {
        if (++steps > P->stepLimit_)
            throw Err(line, "code chạy quá lâu: có thể bạn đã viết một vòng lặp vô hạn (điều kiện dừng không bao giờ đúng)");
    }
    Frame& F() { return frames.back(); }
    void Declare(const std::string& name, const Slot& s, int line) {
        auto& sc = F().scopes.back();
        if (sc.count(name)) throw Err(line, "biến '" + name + "' đã được khai báo trong phạm vi này rồi");
        sc[name] = s;
    }
};

// =============================================================================
//  PARSER
// =============================================================================
class Parser {
public:
    Parser(std::vector<Tok> toks, Program::Impl& impl, std::map<std::string, std::shared_ptr<StructDef>>& structs)
        : T(std::move(toks)), I(impl), S(structs) {}

    void ParseProgram() {
        while (!AtEnd()) {
            if (Accept(";")) continue;
            const Tok& t = Peek();
            if (t.t == TT::Ident && (t.s == "struct" || t.s == "class")) { ParseStruct(); continue; }
            if (t.t == TT::Ident && t.s == "enum") { ParseEnum(); continue; }
            if (t.t == TT::Ident && t.s == "using") { while (!AtEnd() && !Is(";")) Next(); Expect(";", "thiếu dấu ';'"); continue; }
            if (t.t == TT::Ident && t.s == "template") throw Err(t.line, "template chưa được hỗ trợ trong trình chạy thử của game");
            if (t.t == TT::Ident && t.s == "namespace") throw Err(t.line, "namespace tự viết chưa được hỗ trợ trong trình chạy thử");
            if (!IsTypeStart(0)) throw Err(t.line, "không hiểu '" + t.s + "' ở đây. Ngoài hàm chỉ được khai báo hàm, struct/class, enum hoặc hằng số");
            TypeP type = ParseType();
            const Tok& nameTok = Next();
            if (nameTok.t != TT::Ident) throw Err(nameTok.line, "thiếu tên hàm hoặc tên biến sau kiểu " + type->Name());
            if (Is("(")) {
                ParseFunction(type, nameTok.s, nameTok.line, nullptr);
            } else {
                auto st = MakeS(SK::Decl, nameTok.line);
                st->type = type;
                ParseDeclarators(st, nameTok);
                I.globalDecls.push_back(st);
            }
        }
    }

private:
    std::vector<Tok> T;
    size_t p = 0;
    Program::Impl& I;
    std::map<std::string, std::shared_ptr<StructDef>>& S;

    const Tok& Peek(size_t k = 0) const { return T[std::min(p + k, T.size() - 1)]; }
    const Tok& Prev() const { return T[p > 0 ? p - 1 : 0]; }
    const Tok& Next() { const Tok& t = T[p]; if (p < T.size() - 1) p++; return t; }
    bool AtEnd() const { return T[p].t == TT::End; }
    bool Is(const char* s, size_t k = 0) const { const Tok& t = Peek(k); return (t.t == TT::Op || t.t == TT::Ident) && t.s == s; }
    bool Accept(const char* s) { if (Is(s)) { Next(); return true; } return false; }
    void Expect(const char* s, const std::string& msg) {
        if (!Accept(s)) {
            bool semi = std::string(s) == ";";
            int line = semi ? Prev().line : Peek().line;
            std::string near = AtEnd() ? "cuối file" : "'" + Peek().s + "'";
            throw Err(line, msg + (semi ? "" : " (gặp " + near + ")"));
        }
    }

    bool IsStructName(const std::string& n) const { return S.count(n) || n == "vec3" || n == "Vector3"; }

    bool IsTypeStart(size_t k) const {
        const Tok& t = Peek(k);
        if (t.t != TT::Ident) return false;
        static const std::set<std::string> kw = { "int", "float", "double", "bool", "void", "auto", "long", "short",
                                                   "unsigned", "signed", "char", "const", "static", "inline", "constexpr",
                                                   "virtual", "size_t", "vector", "vec3", "Vector3" };
        if (kw.count(t.s)) return true;
        if (t.s == "std" && Is("::", k + 1)) return Peek(k + 2).s == "vector" || Peek(k + 2).s == "size_t";
        if (S.count(t.s) || I.enumTypes.count(t.s)) {
            const Tok& n = Peek(k + 1);
            return n.t == TT::Ident || (n.t == TT::Op && (n.s == "&" || n.s == "*"));
        }
        return false;
    }

    TypeP ParseType() {
        bool isConst = false;
        while (Is("const") || Is("static") || Is("inline") || Is("constexpr") || Is("virtual")) {
            if (Peek().s == "const" || Peek().s == "constexpr") isConst = true;
            Next();
        }
        if (Is("std") && Is("::", 1)) { Next(); Next(); }
        const Tok& t = Next();
        TypeP ty;
        if (t.s == "int" || t.s == "long" || t.s == "short" || t.s == "char" || t.s == "size_t" || t.s == "signed") {
            ty = MakeT(Kind::Int);
            while (Is("int") || Is("long") || Is("short")) Next();
        } else if (t.s == "unsigned") {
            ty = MakeT(Kind::Int);
            while (Is("int") || Is("long") || Is("short") || Is("char")) Next();
        } else if (t.s == "float" || t.s == "double") {
            ty = MakeT(Kind::Float);
        } else if (t.s == "bool") {
            ty = MakeT(Kind::Bool);
        } else if (t.s == "void") {
            ty = MakeT(Kind::Void);
        } else if (t.s == "auto") {
            ty = MakeT(Kind::Void);
            ty->isAuto = true;
        } else if (t.s == "vector") {
            Expect("<", "thiếu '<' sau vector (ví dụ: std::vector<int>)");
            ty = MakeT(Kind::Vector);
            ty->elem = ParseType();
            Expect(">", "thiếu '>' đóng kiểu vector");
        } else if (t.s == "vec3" || t.s == "Vector3") {
            ty = MakeT(Kind::Struct);
            ty->sdef = I.vec3.get();
        } else if (S.count(t.s)) {
            ty = MakeT(Kind::Struct);
            ty->sdef = S[t.s].get();
        } else if (I.enumTypes.count(t.s)) {
            ty = MakeT(Kind::Int);
        } else if (t.s == "string") {
            throw Err(t.line, "kiểu std::string chưa được hỗ trợ trong trình chạy thử (chỉ dùng chuỗi trong std::cout)");
        } else {
            throw Err(t.line, "không biết kiểu dữ liệu '" + t.s + "'");
        }
        if (Accept("const")) isConst = true;
        if (Is("*")) throw Err(Peek().line, "con trỏ (*) chưa được hỗ trợ trong trình chạy thử. Hãy dùng tham chiếu (&)");
        if (Accept("&")) ty->isRef = true;
        ty->isConst = isConst;
        return ty;
    }

    void ParseDeclarators(const StmtP& st, const Tok& firstName) {
        const Tok* nameTok = &firstName;
        while (true) {
            Decl d{ nameTok->s, nullptr, nameTok->line };
            if (Accept("=")) d.init = ParseAssign();
            else if (Is("{")) d.init = ParsePrimary();
            else if (Is("(")) throw Err(Peek().line, "khởi tạo kiểu T x(...) chưa được hỗ trợ, hãy viết T x = ...;");
            else if (Is("[")) throw Err(Peek().line, "mảng kiểu C (int a[10]) chưa được hỗ trợ, hãy dùng std::vector");
            st->decls.push_back(d);
            if (!Accept(",")) break;
            Accept("&");
            const Tok& nt = Next();
            if (nt.t != TT::Ident) throw Err(nt.line, "thiếu tên biến sau dấu ','");
            nameTok = &nt;
        }
        Expect(";", "thiếu dấu ';' ở cuối khai báo");
    }

    std::shared_ptr<FuncDef> ParseFunction(TypeP ret, const std::string& name, int line, StructDef* owner) {
        auto fn = std::make_shared<FuncDef>();
        fn->name = name;
        fn->ret = ret;
        fn->line = line;
        fn->owner = owner;
        Expect("(", "thiếu '(' sau tên hàm");
        if (!Is(")")) {
            if (Is("void") && Is(")", 1)) Next();
            else {
                while (true) {
                    Param prm;
                    prm.type = ParseType();
                    if (Peek().t == TT::Ident) prm.name = Next().s;
                    if (Accept("=")) prm.def = ParseAssign();
                    fn->params.push_back(prm);
                    if (!Accept(",")) break;
                }
            }
        }
        Expect(")", "thiếu ')' đóng danh sách tham số");
        while (Is("const") || Is("override") || Is("final") || Is("noexcept")) Next();
        if (Accept("=")) {                         // = 0, = default
            Next();
            Expect(";", "thiếu dấu ';'");
            return nullptr;
        }
        if (Accept(";")) return nullptr;           // khai báo trước (prototype)
        if (!Is("{")) throw Err(Peek().line, "thiếu '{' mở thân hàm " + name);
        fn->body = ParseBlock();
        if (owner) owner->methods[name] = fn;
        else {
            if (I.funcs.count(name)) throw Err(line, "hàm '" + name + "' bị định nghĩa hai lần (trình chạy thử chưa hỗ trợ nạp chồng hàm)");
            I.funcs[name] = fn;
        }
        return fn;
    }

    void ParseStruct() {
        int line = Next().line;
        const Tok& nameTok = Next();
        if (nameTok.t != TT::Ident) throw Err(nameTok.line, "thiếu tên struct/class");
        auto sd = std::make_shared<StructDef>();
        sd->name = nameTok.s;
        if (S.count(sd->name)) throw Err(line, "struct/class '" + sd->name + "' bị định nghĩa hai lần");
        S[sd->name] = sd;
        if (Accept(":")) {
            Accept("public"); Accept("private"); Accept("protected");
            sd->base = Next().s;
            if (S.count(sd->base) && S[sd->base] != sd) {        // kế thừa trường và hàm của lớp cha
                const StructDef& b = *S[sd->base];
                sd->fields = b.fields;
                sd->methods = b.methods;
            }
        }
        Expect("{", "thiếu '{' mở thân struct/class");
        while (!Is("}")) {
            if (AtEnd()) throw Err(line, "struct/class '" + sd->name + "' chưa được đóng bằng '};'");
            if ((Is("public") || Is("private") || Is("protected")) && Is(":", 1)) { Next(); Next(); continue; }
            if (Accept(";")) continue;
            if (Is("~") || (Is("virtual") && Is("~", 1))) {      // hàm hủy: bỏ qua
                while (!AtEnd() && !Is(";") && !Is("{")) Next();
                if (Is("{")) ParseBlock(); else Next();
                continue;
            }
            if (Peek().s == sd->name && Is("(", 1))
                throw Err(Peek().line, "hàm khởi tạo (constructor) chưa được hỗ trợ trong trình chạy thử. Hãy gán giá trị mặc định khi khai báo biến thành viên, ví dụ: int mau = 100;");
            TypeP ty = ParseType();
            const Tok& mn = Next();
            if (mn.t != TT::Ident) throw Err(mn.line, "thiếu tên biến hoặc hàm thành viên");
            if (Is("(")) {
                ParseFunction(ty, mn.s, mn.line, sd.get());
            } else {
                const Tok* nt = &mn;
                while (true) {
                    FieldDef fd{ ty, nt->s, nullptr };
                    if (Accept("=")) fd.init = ParseAssign();
                    else if (Is("{")) fd.init = ParsePrimary();
                    int idx = sd->FieldIndex(fd.name);
                    if (idx >= 0) sd->fields[idx] = fd; else sd->fields.push_back(fd);
                    if (!Accept(",")) break;
                    const Tok& n2 = Next();
                    nt = &n2;
                }
                Expect(";", "thiếu dấu ';' sau biến thành viên");
            }
        }
        Expect("}", "thiếu '}'");
        Expect(";", "thiếu dấu ';' sau dấu '}' đóng struct/class");
    }

    void ParseEnum() {
        int line = Next().line;
        Accept("class"); Accept("struct");
        std::string name;
        if (Peek().t == TT::Ident) name = Next().s;
        if (Accept(":")) ParseType();
        if (!name.empty()) I.enumTypes.insert(name);
        Expect("{", "thiếu '{' sau enum");
        long long v = 0;
        while (!Is("}")) {
            const Tok& t = Next();
            if (t.t != TT::Ident) throw Err(t.line, "tên giá trị enum không hợp lệ");
            if (Accept("=")) {
                bool neg = Accept("-");
                const Tok& num = Next();
                if (num.t != TT::Int) throw Err(num.line, "giá trị enum phải là số nguyên");
                v = neg ? -num.iv : num.iv;
            }
            I.enumConsts[t.s] = v;
            if (!name.empty()) I.enumConsts[name + "::" + t.s] = v;
            v++;
            if (!Accept(",")) break;
        }
        Expect("}", "thiếu '}' đóng enum");
        Expect(";", "thiếu dấu ';' sau enum");
        (void)line;
    }

    // ---------------- Câu lệnh ----------------
    StmtP ParseBlock() {
        auto b = MakeS(SK::Block, Peek().line);
        Expect("{", "thiếu '{'");
        while (!Is("}")) {
            if (AtEnd()) throw Err(b->line, "thiếu dấu '}' để đóng khối mở ở dòng " + std::to_string(b->line));
            b->body.push_back(ParseStatement());
        }
        Next();
        return b;
    }

    StmtP ParseStatement() {
        const Tok& t = Peek();
        int line = t.line;
        if (Is("{")) return ParseBlock();
        if (Accept(";")) return MakeS(SK::Empty, line);
        if (t.t == TT::Ident) {
            if (t.s == "if") {
                Next();
                auto s = MakeS(SK::If, line);
                Expect("(", "thiếu '(' sau if");
                s->e1 = ParseExpr();
                Expect(")", "thiếu ')' đóng điều kiện if");
                s->s1 = ParseStatement();
                if (Accept("else")) s->s2 = ParseStatement();
                return s;
            }
            if (t.s == "while") {
                Next();
                auto s = MakeS(SK::While, line);
                Expect("(", "thiếu '(' sau while");
                s->e1 = ParseExpr();
                Expect(")", "thiếu ')' đóng điều kiện while");
                s->s1 = ParseStatement();
                return s;
            }
            if (t.s == "do") {
                Next();
                auto s = MakeS(SK::DoWhile, line);
                s->s1 = ParseStatement();
                Expect("while", "thiếu 'while' sau khối do");
                Expect("(", "thiếu '('");
                s->e1 = ParseExpr();
                Expect(")", "thiếu ')'");
                Expect(";", "thiếu dấu ';' sau do-while");
                return s;
            }
            if (t.s == "for") return ParseFor();
            if (t.s == "switch") return ParseSwitch();
            if (t.s == "break") { Next(); Expect(";", "thiếu dấu ';' sau break"); return MakeS(SK::Break, line); }
            if (t.s == "continue") { Next(); Expect(";", "thiếu dấu ';' sau continue"); return MakeS(SK::Continue, line); }
            if (t.s == "return") {
                Next();
                auto s = MakeS(SK::Return, line);
                if (!Is(";")) s->e1 = ParseExpr();
                Expect(";", "thiếu dấu ';' sau return");
                return s;
            }
            if (t.s == "else") throw Err(line, "'else' không đi kèm 'if' nào (kiểm tra lại dấu ngoặc { })");
            if (t.s == "case" || t.s == "default") throw Err(line, "'" + t.s + "' chỉ dùng bên trong switch");
            if (IsTypeStart(0)) {
                auto s = MakeS(SK::Decl, line);
                s->type = ParseType();
                const Tok& nt = Next();
                if (nt.t != TT::Ident) throw Err(nt.line, "thiếu tên biến sau kiểu " + s->type->Name());
                ParseDeclarators(s, nt);
                return s;
            }
        }
        auto s = MakeS(SK::ExprS, line);
        s->e1 = ParseExpr();
        Expect(";", "thiếu dấu ';' ở cuối câu lệnh");
        return s;
    }

    StmtP ParseFor() {
        int line = Next().line;
        Expect("(", "thiếu '(' sau for");
        // Range-for: for (kiểu tên : vector)
        if (IsTypeStart(0)) {
            size_t save = p;
            TypeP ty = ParseType();
            if (Peek().t == TT::Ident && Is(":", 1)) {
                auto s = MakeS(SK::RangeFor, line);
                s->type = ty;
                s->name = Next().s;
                Next();
                s->e1 = ParseExpr();
                Expect(")", "thiếu ')' trong vòng for");
                s->s1 = ParseStatement();
                return s;
            }
            p = save;
        }
        auto s = MakeS(SK::For, line);
        if (!Accept(";")) {
            if (IsTypeStart(0)) {
                auto d = MakeS(SK::Decl, Peek().line);
                d->type = ParseType();
                const Tok& nt = Next();
                ParseDeclarators(d, nt);
                s->s1 = d;
            } else {
                auto d = MakeS(SK::ExprS, Peek().line);
                d->e1 = ParseExpr();
                Expect(";", "thiếu dấu ';' trong vòng for");
                s->s1 = d;
            }
        }
        if (!Is(";")) s->e1 = ParseExpr();
        Expect(";", "thiếu dấu ';' thứ hai trong vòng for");
        if (!Is(")")) s->e2 = ParseExpr();
        Expect(")", "thiếu ')' đóng vòng for");
        s->s2 = ParseStatement();
        return s;
    }

    StmtP ParseSwitch() {
        int line = Next().line;
        auto s = MakeS(SK::Switch, line);
        Expect("(", "thiếu '(' sau switch");
        s->e1 = ParseExpr();
        Expect(")", "thiếu ')'");
        Expect("{", "thiếu '{' sau switch");
        while (!Is("}")) {
            if (AtEnd()) throw Err(line, "switch chưa được đóng bằng '}'");
            if (Accept("case")) {
                ExprP label = ParseTernary();
                Expect(":", "thiếu ':' sau case");
                s->cases.push_back({ label, s->body.size() });
            } else if (Accept("default")) {
                Expect(":", "thiếu ':' sau default");
                s->cases.push_back({ nullptr, s->body.size() });
            } else {
                s->body.push_back(ParseStatement());
            }
        }
        Next();
        return s;
    }

    // ---------------- Biểu thức ----------------
    ExprP ParseExpr() { return ParseAssign(); }

    ExprP ParseAssign() {
        ExprP left = ParseTernary();
        static const std::set<std::string> aops = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=" };
        const Tok& t = Peek();
        if (t.t == TT::Op && aops.count(t.s)) {
            Next();
            auto e = MakeE(EK::Assign, t.line);
            e->op = t.s;
            e->a = { left, ParseAssign() };
            return e;
        }
        return left;
    }

    ExprP ParseTernary() {
        ExprP c = ParseBinary(0);
        if (Is("?")) {
            int line = Next().line;
            auto e = MakeE(EK::Ternary, line);
            ExprP a = ParseAssign();
            Expect(":", "thiếu ':' trong toán tử ba ngôi ? :");
            ExprP b = ParseAssign();
            e->a = { c, a, b };
            return e;
        }
        return c;
    }

    int Prec(const std::string& op) const {
        static const std::map<std::string, int> P = {
            { "||", 1 }, { "&&", 2 }, { "|", 3 }, { "^", 4 }, { "&", 5 }, { "==", 6 }, { "!=", 6 },
            { "<", 7 }, { ">", 7 }, { "<=", 7 }, { ">=", 7 }, { "<<", 8 }, { ">>", 8 },
            { "+", 9 }, { "-", 9 }, { "*", 10 }, { "/", 10 }, { "%", 10 } };
        auto it = P.find(op);
        return it == P.end() ? -1 : it->second;
    }

    ExprP ParseBinary(int minPrec) {
        ExprP left = ParseUnary();
        while (true) {
            const Tok& t = Peek();
            if (t.t != TT::Op) break;
            int pr = Prec(t.s);
            if (pr < 0 || pr < minPrec) break;
            Next();
            ExprP right = ParseBinary(pr + 1);
            auto e = MakeE((t.s == "&&" || t.s == "||") ? EK::Logical : EK::Binary, t.line);
            e->op = t.s;
            e->a = { left, right };
            left = e;
        }
        return left;
    }

    bool IsSimpleTypeKw(const Tok& t) const {
        return t.t == TT::Ident && (t.s == "int" || t.s == "float" || t.s == "double" || t.s == "bool" || t.s == "long" ||
                                   t.s == "unsigned" || t.s == "char" || t.s == "short" || t.s == "size_t");
    }

    ExprP ParseUnary() {
        const Tok& t = Peek();
        if (t.t == TT::Op) {
            if (t.s == "!" || t.s == "-" || t.s == "+" || t.s == "~") {
                Next();
                auto e = MakeE(EK::Unary, t.line);
                e->op = t.s;
                e->a = { ParseUnary() };
                return e;
            }
            if (t.s == "++" || t.s == "--") {
                Next();
                auto e = MakeE(EK::IncDec, t.line);
                e->op = t.s;
                e->prefix = true;
                e->a = { ParseUnary() };
                return e;
            }
            if (t.s == "*" || t.s == "&") throw Err(t.line, "toán tử con trỏ '" + t.s + "' chưa được hỗ trợ trong trình chạy thử");
            if (t.s == "(" && (IsSimpleTypeKw(Peek(1)) || (Peek(1).t == TT::Ident && IsStructName(Peek(1).s) && Is(")", 2)))) {
                Next();
                auto e = MakeE(EK::Cast, t.line);
                e->type = ParseType();
                Expect(")", "thiếu ')' trong phép ép kiểu");
                e->a = { ParseUnary() };
                return e;
            }
        }
        return ParsePostfix(ParsePrimary());
    }

    ExprP ParsePostfix(ExprP e) {
        while (true) {
            const Tok& t = Peek();
            if (t.t != TT::Op) break;
            if (t.s == "(") {
                if (e->k != EK::Var) throw Err(t.line, "chỉ gọi được hàm theo tên");
                Next();
                auto c = MakeE(EK::Call, t.line);
                c->name = e->name;
                c->a = ParseArgs();
                e = c;
            } else if (t.s == "." || t.s == "->") {
                Next();
                const Tok& m = Next();
                if (m.t != TT::Ident) throw Err(m.line, "thiếu tên thành viên sau '" + t.s + "'");
                if (Is("(")) {
                    Next();
                    auto c = MakeE(EK::Method, m.line);
                    c->name = m.s;
                    c->a = ParseArgs();
                    c->a.insert(c->a.begin(), e);
                    e = c;
                } else {
                    auto c = MakeE(EK::Member, m.line);
                    c->name = m.s;
                    c->a = { e };
                    e = c;
                }
            } else if (t.s == "[") {
                Next();
                auto c = MakeE(EK::Index, t.line);
                c->a = { e, ParseExpr() };
                Expect("]", "thiếu ']'");
                e = c;
            } else if (t.s == "++" || t.s == "--") {
                Next();
                auto c = MakeE(EK::IncDec, t.line);
                c->op = t.s;
                c->prefix = false;
                c->a = { e };
                e = c;
            } else break;
        }
        return e;
    }

    std::vector<ExprP> ParseArgs() {
        std::vector<ExprP> args;
        if (!Is(")")) {
            while (true) {
                args.push_back(ParseAssign());
                if (!Accept(",")) break;
            }
        }
        Expect(")", "thiếu ')' đóng lời gọi hàm");
        return args;
    }

    ExprP ParsePrimary() {
        const Tok& t = Next();
        int line = t.line;
        if (t.t == TT::Int) { auto e = MakeE(EK::Int, line); e->ival = t.iv; return e; }
        if (t.t == TT::Float) { auto e = MakeE(EK::Float, line); e->fval = t.fv; return e; }
        if (t.t == TT::Str) { auto e = MakeE(EK::Str, line); e->name = t.s; return e; }
        if (t.t == TT::Op && t.s == "(") {
            ExprP e = ParseExpr();
            Expect(")", "thiếu dấu ')'");
            return e;
        }
        if (t.t == TT::Op && t.s == "{") {
            auto e = MakeE(EK::InitList, line);
            if (!Is("}")) {
                while (true) {
                    e->a.push_back(ParseAssign());
                    if (!Accept(",")) break;
                    if (Is("}")) break;
                }
            }
            Expect("}", "thiếu '}' đóng danh sách khởi tạo");
            return e;
        }
        if (t.t != TT::Ident) throw Err(line, "thiếu giá trị hoặc biểu thức (gặp '" + t.s + "')");
        if (t.s == "true" || t.s == "false") { auto e = MakeE(EK::Bool, line); e->ival = t.s == "true"; return e; }
        if (t.s == "this") return MakeE(EK::This, line);
        if (t.s == "nullptr" || t.s == "new" || t.s == "delete") throw Err(line, "'" + t.s + "' (con trỏ / cấp phát động) chưa được hỗ trợ trong trình chạy thử");
        if (t.s == "static_cast") {
            Expect("<", "thiếu '<' sau static_cast");
            auto e = MakeE(EK::Cast, line);
            e->type = ParseType();
            Expect(">", "thiếu '>'");
            Expect("(", "thiếu '('");
            e->a = { ParseExpr() };
            Expect(")", "thiếu ')'");
            return e;
        }
        if (IsSimpleTypeKw(t) && Is("(")) {                    // float(x)
            p--;
            auto e = MakeE(EK::Cast, line);
            e->type = ParseType();
            Expect("(", "thiếu '('");
            e->a = { ParseExpr() };
            Expect(")", "thiếu ')'");
            return e;
        }
        if (IsStructName(t.s) && (Is("{") || Is("("))) {      // Hop{...} hoặc vec3(...)
            auto e = MakeE(EK::Construct, line);
            e->type = MakeT(Kind::Struct);
            e->type->sdef = (t.s == "vec3" || t.s == "Vector3") ? I.vec3.get() : S[t.s].get();
            bool brace = Is("{");
            Next();
            if (!Is(brace ? "}" : ")")) {
                while (true) {
                    e->a.push_back(ParseAssign());
                    if (!Accept(",")) break;
                }
            }
            Expect(brace ? "}" : ")", brace ? "thiếu '}'" : "thiếu ')'");
            if (!brace && e->type->sdef != I.vec3.get())
                throw Err(line, "hàm khởi tạo (constructor) chưa được hỗ trợ, hãy viết " + t.s + "{...}");
            return e;
        }
        std::string name = t.s;
        while (Is("::")) {                                     // std::max, Huong::Trai
            Next();
            const Tok& n2 = Next();
            if (n2.t != TT::Ident) throw Err(n2.line, "thiếu tên sau '::'");
            if (name == "std") name = n2.s;
            else name = name + "::" + n2.s;
        }
        auto e = MakeE(EK::Var, line);
        e->name = name;
        return e;
    }
};

// =============================================================================
//  INTERPRETER
// =============================================================================
Value Program::Impl::Default(const TypeInfo& t) {
    Value v;
    v.kind = t.kind;
    if (t.kind == Kind::Struct && t.sdef) {
        v.sdef = t.sdef;
        for (const FieldDef& fd : t.sdef->fields) {
            Slot s = std::make_shared<Value>(Default(*fd.type));
            if (fd.init) Store(s, Eval(fd.init), 0);
            v.items.push_back(s);
        }
    } else if (t.kind == Kind::Vector) {
        v.elem = t.elem;
    }
    return v;
}

Value Program::Impl::Copy(const Value& v) {
    Value r = v;
    r.readonly = false;
    if (v.kind == Kind::Struct || v.kind == Kind::Vector) {
        r.items.clear();
        for (const Slot& s : v.items) r.items.push_back(std::make_shared<Value>(Copy(*s)));
    }
    return r;
}

static std::string KindName(const Value& v) {
    switch (v.kind) {
    case Kind::Int: return "int";
    case Kind::Float: return "float";
    case Kind::Bool: return "bool";
    case Kind::Struct: return v.sdef ? v.sdef->name : "struct";
    case Kind::Vector: return "vector";
    case Kind::Str: return "chuỗi";
    case Kind::Void: return "void";
    default: return "?";
    }
}

Value Program::Impl::Convert(const Value& v, const TypeInfo& t, int line) {
    if (t.isAuto) {
        if (v.kind == Kind::InitList) throw Err(line, "không dùng auto với {...}, hãy ghi rõ kiểu");
        return Copy(v);
    }
    switch (t.kind) {
    case Kind::Int:
        if (!v.IsNumber()) throw Err(line, "không thể đổi " + KindName(v) + " sang int");
        return Value::Int(v.AsInt());
    case Kind::Float:
        if (!v.IsNumber()) throw Err(line, "không thể đổi " + KindName(v) + " sang float");
        return Value::Float(v.AsFloat());
    case Kind::Bool:
        if (!v.IsNumber()) throw Err(line, "không thể đổi " + KindName(v) + " sang bool");
        return Value::Bool(v.AsBool());
    case Kind::Struct: {
        if (v.kind == Kind::InitList) {
            Value r = Default(t);
            if (t.sdef == vec3.get() && v.items.size() == 1) {
                for (int k = 0; k < 3; ++k) Store(r.items[k], *v.items[0], line);
                return r;
            }
            if (v.items.size() > r.items.size()) throw Err(line, "quá nhiều giá trị khởi tạo cho " + t.sdef->name);
            for (size_t k = 0; k < v.items.size(); ++k) Store(r.items[k], *v.items[k], line);
            return r;
        }
        if (v.kind != Kind::Struct || v.sdef != t.sdef)
            throw Err(line, "kiểu không khớp: cần " + t.Name() + " nhưng nhận " + KindName(v));
        return Copy(v);
    }
    case Kind::Vector: {
        Value r = Default(t);
        if (v.kind == Kind::InitList || v.kind == Kind::Vector) {
            for (const Slot& it : v.items) r.items.push_back(std::make_shared<Value>(Convert(*it, *t.elem, line)));
            return r;
        }
        throw Err(line, "kiểu không khớp: cần vector nhưng nhận " + KindName(v));
    }
    case Kind::Void: return Value();
    default: return v;
    }
}

void Program::Impl::Store(const Slot& dst, const Value& v, int line) {
    if (dst->readonly) throw Err(line, "không thể thay đổi hằng số (const)");
    switch (dst->kind) {
    case Kind::Int:
        if (!v.IsNumber()) throw Err(line, "không thể gán " + KindName(v) + " cho biến int");
        dst->i = v.AsInt();
        return;
    case Kind::Float:
        if (!v.IsNumber()) throw Err(line, "không thể gán " + KindName(v) + " cho biến float");
        dst->f = v.AsFloat();
        return;
    case Kind::Bool:
        if (!v.IsNumber()) throw Err(line, "không thể gán " + KindName(v) + " cho biến bool");
        dst->i = v.AsBool() ? 1 : 0;
        return;
    case Kind::Struct: {
        TypeInfo t;
        t.kind = Kind::Struct;
        t.sdef = dst->sdef;
        Value nv = Convert(v, t, line);
        for (size_t k = 0; k < dst->items.size() && k < nv.items.size(); ++k) Store(dst->items[k], *nv.items[k], line);
        return;
    }
    case Kind::Vector: {
        TypeInfo t;
        t.kind = Kind::Vector;
        t.elem = dst->elem;
        Value nv = Convert(v, t, line);
        dst->items = nv.items;
        return;
    }
    default:
        *dst = Copy(v);
    }
}

void Program::Impl::EnsureGlobals() {
    if (globalsReady) return;
    globalsReady = true;
    frames.push_back(Frame{});
    F().scopes.emplace_back();
    for (const StmtP& d : globalDecls) {
        for (const Decl& dc : d->decls) {
            Slot s = std::make_shared<Value>(Default(*d->type));
            if (dc.init) {
                Value v = Eval(dc.init);
                if (d->type->isAuto) *s = Copy(v); else Store(s, v, dc.line);
            }
            s->readonly = d->type->isConst;
            globals[dc.name] = s;
        }
    }
    frames.pop_back();
}

Slot Program::Impl::Lookup(const std::string& name, int line) {
    Frame& fr = F();
    for (auto it = fr.scopes.rbegin(); it != fr.scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    if (fr.self && fr.self->sdef) {
        int idx = fr.self->sdef->FieldIndex(name);
        if (idx >= 0) return fr.self->items[idx];
    }
    auto g = globals.find(name);
    if (g != globals.end()) return g->second;
    auto en = enumConsts.find(name);
    if (en != enumConsts.end()) { auto s = std::make_shared<Value>(Value::Int(en->second)); s->readonly = true; return s; }
    auto pc = P->constants_.find(name);
    if (pc != P->constants_.end()) { auto s = std::make_shared<Value>(Value::Int(pc->second)); s->readonly = true; return s; }
    if (name == "PI" || name == "M_PI") { auto s = std::make_shared<Value>(Value::Float(3.14159265358979)); s->readonly = true; return s; }
    if (name == "cout") { auto s = std::make_shared<Value>(); s->kind = Kind::Cout; return s; }
    if (name == "endl") { auto s = std::make_shared<Value>(); s->kind = Kind::Str; s->s = "\n"; return s; }
    throw Err(line, "'" + name + "' chưa được khai báo (viết sai tên? quên khai báo biến?)");
}

Slot Program::Impl::EvalRef(const ExprP& e) {
    switch (e->k) {
    case EK::Var: return Lookup(e->name, e->line);
    case EK::This:
        if (!F().self) throw Err(e->line, "'this' chỉ dùng trong hàm thành viên");
        return F().self;
    case EK::Member: {
        Slot obj = EvalRef(e->a[0]);
        if (obj->kind != Kind::Struct) throw Err(e->line, "'" + e->name + "': chỉ struct/class mới có thành viên (dùng dấu chấm)");
        int idx = obj->sdef->FieldIndex(e->name);
        if (idx < 0) throw Err(e->line, obj->sdef->name + " không có thành viên '" + e->name + "'");
        return obj->items[idx];
    }
    case EK::Index: {
        Slot arr = EvalRef(e->a[0]);
        long long idx = Eval(e->a[1]).AsInt();
        if (arr->kind != Kind::Vector) throw Err(e->line, "chỉ dùng [ ] với std::vector");
        if (idx < 0 || idx >= (long long)arr->items.size())
            throw Err(e->line, "truy cập ngoài mảng: chỉ số " + std::to_string(idx) + " nhưng vector chỉ có " +
                                   std::to_string(arr->items.size()) + " phần tử");
        return arr->items[(size_t)idx];
    }
    default:
        return std::make_shared<Value>(Eval(e));    // giá trị tạm (không phải biến)
    }
}

static bool IsLvalue(const ExprP& e) { return e->k == EK::Var || e->k == EK::Member || e->k == EK::Index || e->k == EK::This; }

Value Program::Impl::BinaryOp(const std::string& op, const Value& a, const Value& b, int line) {
    // vec3 (kiểu GLSL)
    bool av = a.kind == Kind::Struct && a.sdef == vec3.get(), bv = b.kind == Kind::Struct && b.sdef == vec3.get();
    if (av || bv) {
        if (op != "+" && op != "-" && op != "*" && op != "/") throw Err(line, "phép '" + op + "' không dùng được với vec3");
        Value r = Default(*[&] { auto t = MakeT(Kind::Struct); t->sdef = vec3.get(); return t; }());
        for (int k = 0; k < 3; ++k) {
            double x = av ? a.items[k]->f : a.AsFloat();
            double y = bv ? b.items[k]->f : b.AsFloat();
            double z = op == "+" ? x + y : op == "-" ? x - y : op == "*" ? x * y : (y == 0 ? 0 : x / y);
            r.items[k]->f = z;
        }
        return r;
    }
    if (a.kind == Kind::Cout && op == "<<") {
        if (P->output_.size() < 6000) P->output_ += b.ToString();
        return a;
    }
    if (!a.IsNumber() || !b.IsNumber())
        throw Err(line, "phép '" + op + "' không dùng được giữa " + KindName(a) + " và " + KindName(b));
    bool fl = a.kind == Kind::Float || b.kind == Kind::Float;
    if (op == "==") return Value::Bool(fl ? a.AsFloat() == b.AsFloat() : a.i == b.i);
    if (op == "!=") return Value::Bool(fl ? a.AsFloat() != b.AsFloat() : a.i != b.i);
    if (op == "<") return Value::Bool(fl ? a.AsFloat() < b.AsFloat() : a.i < b.i);
    if (op == ">") return Value::Bool(fl ? a.AsFloat() > b.AsFloat() : a.i > b.i);
    if (op == "<=") return Value::Bool(fl ? a.AsFloat() <= b.AsFloat() : a.i <= b.i);
    if (op == ">=") return Value::Bool(fl ? a.AsFloat() >= b.AsFloat() : a.i >= b.i);
    if (op == "%") {
        if (fl) throw Err(line, "phép % chỉ dùng cho số nguyên (với số thực hãy dùng fmod)");
        if (b.i == 0) throw Err(line, "chia lấy dư cho 0!");
        return Value::Int(a.i % b.i);
    }
    if (op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
        if (fl) throw Err(line, "phép '" + op + "' chỉ dùng cho số nguyên");
        long long r = op == "&" ? (a.i & b.i) : op == "|" ? (a.i | b.i) : op == "^" ? (a.i ^ b.i) : op == "<<" ? (a.i << b.i) : (a.i >> b.i);
        return Value::Int(r);
    }
    if (fl) {
        double x = a.AsFloat(), y = b.AsFloat();
        if (op == "+") return Value::Float(x + y);
        if (op == "-") return Value::Float(x - y);
        if (op == "*") return Value::Float(x * y);
        if (op == "/") {
            if (y == 0) throw Err(line, "chia cho 0!");
            return Value::Float(x / y);
        }
    } else {
        long long x = a.i, y = b.i;
        if (op == "+") return Value::Int(x + y);
        if (op == "-") return Value::Int(x - y);
        if (op == "*") return Value::Int(x * y);
        if (op == "/") {
            if (y == 0) throw Err(line, "chia cho 0!");
            return Value::Int(x / y);                         // chia nguyên: bỏ phần thập phân!
        }
    }
    throw Err(line, "toán tử '" + op + "' chưa được hỗ trợ");
}

Value Program::Impl::Eval(const ExprP& e) {
    switch (e->k) {
    case EK::Int: return Value::Int(e->ival);
    case EK::Float: return Value::Float(e->fval);
    case EK::Bool: return Value::Bool(e->ival != 0);
    case EK::Str: { Value v; v.kind = Kind::Str; v.s = e->name; return v; }
    case EK::Var: case EK::Member: case EK::Index: case EK::This: {
        Slot s = EvalRef(e);
        return *s;
    }
    case EK::Unary: {
        Value v = Eval(e->a[0]);
        if (e->op == "!") return Value::Bool(!v.AsBool());
        if (v.kind == Kind::Struct && v.sdef == vec3.get() && e->op == "-") return BinaryOp("*", v, Value::Float(-1), e->line);
        if (!v.IsNumber()) throw Err(e->line, "phép '" + e->op + "' cần một số");
        if (e->op == "-") return v.kind == Kind::Float ? Value::Float(-v.f) : Value::Int(-v.i);
        if (e->op == "~") return Value::Int(~v.AsInt());
        return v.kind == Kind::Bool ? Value::Int(v.i) : v;
    }
    case EK::Binary: return BinaryOp(e->op, Eval(e->a[0]), Eval(e->a[1]), e->line);
    case EK::Logical: {
        bool l = Eval(e->a[0]).AsBool();
        if (e->op == "&&") return Value::Bool(l && Eval(e->a[1]).AsBool());
        return Value::Bool(l || Eval(e->a[1]).AsBool());
    }
    case EK::Ternary: {
        bool c = Eval(e->a[0]).AsBool();
        Value a = Eval(e->a[c ? 1 : 2]);
        return a;
    }
    case EK::Assign: {
        if (!IsLvalue(e->a[0])) throw Err(e->line, "vế trái của phép gán phải là một biến");
        Slot dst = EvalRef(e->a[0]);
        Value rhs = Eval(e->a[1]);
        if (e->op == "=") Store(dst, rhs, e->line);
        else Store(dst, BinaryOp(e->op.substr(0, e->op.size() - 1), *dst, rhs, e->line), e->line);
        return *dst;
    }
    case EK::IncDec: {
        if (!IsLvalue(e->a[0])) throw Err(e->line, "'" + e->op + "' chỉ dùng với biến");
        Slot dst = EvalRef(e->a[0]);
        if (!dst->IsNumber()) throw Err(e->line, "'" + e->op + "' chỉ dùng với biến số");
        Value old = *dst;
        Store(dst, BinaryOp(e->op == "++" ? "+" : "-", old, Value::Int(1), e->line), e->line);
        return e->prefix ? *dst : old;
    }
    case EK::Cast: {
        Value v = Eval(e->a[0]);
        TypeInfo t = *e->type;
        t.isRef = false;
        return Convert(v, t, e->line);
    }
    case EK::InitList: {
        Value v;
        v.kind = Kind::InitList;
        for (const ExprP& x : e->a) v.items.push_back(std::make_shared<Value>(Eval(x)));
        return v;
    }
    case EK::Construct: {
        Value il;
        il.kind = Kind::InitList;
        for (const ExprP& x : e->a) il.items.push_back(std::make_shared<Value>(Eval(x)));
        return Convert(il, *e->type, e->line);
    }
    case EK::Call: {
        // Hàm thành viên gọi hàm thành viên khác
        Frame& fr = F();
        if (fr.self && fr.self->sdef) {
            auto m = fr.self->sdef->methods.find(e->name);
            if (m != fr.self->sdef->methods.end()) return Invoke(*m->second, {}, &e->a, fr.self, e->line);
        }
        auto f = funcs.find(e->name);
        if (f != funcs.end()) return Invoke(*f->second, {}, &e->a, nullptr, e->line);
        std::vector<Value> args;
        for (const ExprP& x : e->a) args.push_back(Eval(x));
        auto nat = P->natives_.find(e->name);
        if (nat != P->natives_.end()) { Step(e->line); return nat->second(args, e->line); }
        bool found = false;
        Value r = CallBuiltin(e->name, args, e->line, found);
        if (!found) throw Err(e->line, "không tìm thấy hàm '" + e->name + "'");
        return r;
    }
    case EK::Method: {
        Slot obj = EvalRef(e->a[0]);
        std::vector<ExprP> argEx(e->a.begin() + 1, e->a.end());
        if (obj->kind == Kind::Vector) {
            std::vector<Value> args;
            for (const ExprP& x : argEx) args.push_back(Eval(x));
            return VectorMethod(obj, e->name, args, e->line);
        }
        if (obj->kind != Kind::Struct || !obj->sdef) throw Err(e->line, "'" + e->name + "()': chỉ đối tượng struct/class mới có hàm thành viên");
        auto m = obj->sdef->methods.find(e->name);
        if (m == obj->sdef->methods.end()) throw Err(e->line, obj->sdef->name + " không có hàm '" + e->name + "'");
        return Invoke(*m->second, {}, &argEx, obj, e->line);
    }
    }
    throw Err(e->line, "biểu thức không hợp lệ");
}

Value Program::Impl::VectorMethod(const Slot& obj, const std::string& m, std::vector<Value>& args, int line) {
    auto& items = obj->items;
    if (m == "size") return Value::Int((long long)items.size());
    if (m == "empty") return Value::Bool(items.empty());
    if (m == "clear") { items.clear(); return Value(); }
    if (m == "push_back" || m == "emplace_back") {
        if (args.size() != 1) throw Err(line, "push_back cần đúng 1 tham số");
        items.push_back(std::make_shared<Value>(Convert(args[0], *obj->elem, line)));
        return Value();
    }
    if (m == "pop_back") {
        if (items.empty()) throw Err(line, "pop_back trên vector rỗng!");
        items.pop_back();
        return Value();
    }
    if (m == "back" || m == "front") {
        if (items.empty()) throw Err(line, m + "() trên vector rỗng!");
        return m == "back" ? *items.back() : *items.front();
    }
    if (m == "at") {
        long long k = args.empty() ? -1 : args[0].AsInt();
        if (k < 0 || k >= (long long)items.size()) throw Err(line, "at(): chỉ số ngoài mảng");
        return *items[(size_t)k];
    }
    throw Err(line, "std::vector không có hàm '" + m + "' trong trình chạy thử");
}

Value Program::Impl::CallBuiltin(const std::string& n, std::vector<Value>& a, int line, bool& found) {
    found = true;
    auto num = [&](size_t k) {
        if (k >= a.size()) throw Err(line, "hàm " + n + " thiếu tham số");
        if (!a[k].IsNumber()) throw Err(line, "hàm " + n + " cần tham số là số");
        return a[k].AsFloat();
    };
    auto need = [&](size_t cnt) { if (a.size() != cnt) throw Err(line, "hàm " + n + " cần " + std::to_string(cnt) + " tham số"); };
    auto isV = [&](size_t k) { return k < a.size() && a[k].kind == Kind::Struct && a[k].sdef == vec3.get(); };
    auto vx = [&](size_t k, int c) { return a[k].items[c]->f; };
    auto mkV = [&](double x, double y, double z) {
        auto t = MakeT(Kind::Struct); t->sdef = vec3.get();
        Value r = Default(*t);
        r.items[0]->f = x; r.items[1]->f = y; r.items[2]->f = z;
        return r;
    };
    bool allInt = true;
    for (auto& v : a) if (v.kind == Kind::Float) allInt = false;

    std::string s = n;
    if (s.size() > 1 && s.back() == 'f' && (s == "sqrtf" || s == "sinf" || s == "cosf" || s == "tanf" || s == "powf" || s == "expf" ||
                                             s == "fabsf" || s == "fmodf" || s == "fminf" || s == "fmaxf" || s == "floorf" ||
                                             s == "ceilf" || s == "logf" || s == "atan2f" || s == "roundf"))
        s.pop_back();

    if (s == "abs") { need(1); return a[0].kind == Kind::Float ? Value::Float(std::fabs(a[0].f)) : Value::Int(std::llabs(a[0].AsInt())); }
    if (s == "fabs") { need(1); return Value::Float(std::fabs(num(0))); }
    if (s == "sqrt") { need(1); if (num(0) < 0) throw Err(line, "sqrt của số âm!"); return Value::Float(std::sqrt(num(0))); }
    if (s == "sin") { need(1); return Value::Float(std::sin(num(0))); }
    if (s == "cos") { need(1); return Value::Float(std::cos(num(0))); }
    if (s == "tan") { need(1); return Value::Float(std::tan(num(0))); }
    if (s == "exp") { need(1); return Value::Float(std::exp(num(0))); }
    if (s == "log") { need(1); return Value::Float(std::log(num(0))); }
    if (s == "floor") { need(1); return Value::Float(std::floor(num(0))); }
    if (s == "ceil") { need(1); return Value::Float(std::ceil(num(0))); }
    if (s == "round") { need(1); return Value::Float(std::round(num(0))); }
    if (s == "pow") { need(2); return Value::Float(std::pow(num(0), num(1))); }
    if (s == "fmod") { need(2); if (num(1) == 0) throw Err(line, "fmod chia cho 0!"); return Value::Float(std::fmod(num(0), num(1))); }
    if (s == "atan2") { need(2); return Value::Float(std::atan2(num(0), num(1))); }
    if (s == "min" || s == "fmin") {
        need(2);
        return (allInt && s == "min") ? Value::Int(std::min(a[0].AsInt(), a[1].AsInt())) : Value::Float(std::fmin(num(0), num(1)));
    }
    if (s == "max" || s == "fmax") {
        need(2);
        return (allInt && s == "max") ? Value::Int(std::max(a[0].AsInt(), a[1].AsInt())) : Value::Float(std::fmax(num(0), num(1)));
    }
    if (s == "clamp") {
        need(3);
        if (allInt) return Value::Int(std::min(std::max(a[0].AsInt(), a[1].AsInt()), a[2].AsInt()));
        return Value::Float(std::fmin(std::fmax(num(0), num(1)), num(2)));
    }
    if (s == "sign") { need(1); double x = num(0); return Value::Float(x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0)); }
    if (s == "lerp" || s == "mix" || s == "Lerp") {
        need(3);
        if (isV(0) && isV(1)) {
            double t = num(2);
            return mkV(vx(0, 0) + (vx(1, 0) - vx(0, 0)) * t, vx(0, 1) + (vx(1, 1) - vx(0, 1)) * t, vx(0, 2) + (vx(1, 2) - vx(0, 2)) * t);
        }
        return Value::Float(num(0) + (num(1) - num(0)) * num(2));
    }
    if (s == "smoothstep") {
        need(3);
        double t = std::fmin(std::fmax((num(2) - num(0)) / (num(1) - num(0)), 0.0), 1.0);
        return Value::Float(t * t * (3 - 2 * t));
    }
    if (s == "nhieu" || s == "noise") {                     // nhiễu trắng -1..1 (dùng cho âm thanh)
        seed = seed * 1664525u + 1013904223u;
        return Value::Float(((seed >> 9) & 0xFFFF) / 32767.5 - 1.0);
    }
    if (s == "dot") {
        need(2);
        if (!isV(0) || !isV(1)) throw Err(line, "dot cần 2 tham số vec3");
        return Value::Float(vx(0, 0) * vx(1, 0) + vx(0, 1) * vx(1, 1) + vx(0, 2) * vx(1, 2));
    }
    if (s == "length") {
        need(1);
        if (!isV(0)) throw Err(line, "length cần tham số vec3");
        return Value::Float(std::sqrt(vx(0, 0) * vx(0, 0) + vx(0, 1) * vx(0, 1) + vx(0, 2) * vx(0, 2)));
    }
    if (s == "normalize") {
        need(1);
        if (!isV(0)) throw Err(line, "normalize cần tham số vec3");
        double l = std::sqrt(vx(0, 0) * vx(0, 0) + vx(0, 1) * vx(0, 1) + vx(0, 2) * vx(0, 2));
        if (l == 0) return mkV(0, 0, 0);
        return mkV(vx(0, 0) / l, vx(0, 1) / l, vx(0, 2) / l);
    }
    if (s == "cross") {
        need(2);
        if (!isV(0) || !isV(1)) throw Err(line, "cross cần 2 tham số vec3");
        return mkV(vx(0, 1) * vx(1, 2) - vx(0, 2) * vx(1, 1), vx(0, 2) * vx(1, 0) - vx(0, 0) * vx(1, 2), vx(0, 0) * vx(1, 1) - vx(0, 1) * vx(1, 0));
    }
    if (s == "reflect") {
        need(2);
        double d = vx(0, 0) * vx(1, 0) + vx(0, 1) * vx(1, 1) + vx(0, 2) * vx(1, 2);
        return mkV(vx(0, 0) - 2 * d * vx(1, 0), vx(0, 1) - 2 * d * vx(1, 1), vx(0, 2) - 2 * d * vx(1, 2));
    }
    if (s == "printf" || s == "puts") throw Err(line, "hãy dùng std::cout << ... thay cho " + s);
    found = false;
    return Value();
}

Value Program::Impl::Invoke(const FuncDef& fn, const std::vector<Slot>& argSlots, const std::vector<ExprP>* argExprs,
                            const Slot& self, int line) {
    size_t given = argExprs ? argExprs->size() : argSlots.size();
    size_t required = 0;
    for (const Param& p : fn.params) if (!p.def) required++;
    if (given < required || given > fn.params.size())
        throw Err(line, "hàm '" + fn.name + "' cần " + std::to_string(fn.params.size()) + " tham số nhưng được truyền " + std::to_string(given));
    if (frames.size() > 150) throw Err(line, "gọi hàm lồng nhau quá sâu (đệ quy không có điểm dừng?)");

    // Tính tham số trong frame của người gọi
    std::vector<Slot> bound;
    for (size_t k = 0; k < fn.params.size(); ++k) {
        const Param& p = fn.params[k];
        Slot s;
        if (k < given) {
            if (p.type->isRef) {
                if (argExprs) {
                    if (!IsLvalue((*argExprs)[k]) && !p.type->isConst)
                        throw Err(line, "tham số '" + p.name + "' là tham chiếu (&) nên phải truyền vào một biến");
                    s = EvalRef((*argExprs)[k]);
                } else {
                    s = argSlots[k];
                }
                bool ok = s->kind == p.type->kind || p.type->isAuto;
                if (!ok && p.type->isConst) s = std::make_shared<Value>(Convert(*s, *p.type, line));
                else if (!ok) throw Err(line, "tham chiếu '" + p.name + "' cần biến kiểu " + p.type->Name() + " nhưng nhận " + KindName(*s));
            } else {
                Value v = argExprs ? Eval((*argExprs)[k]) : *argSlots[k];
                s = std::make_shared<Value>(Convert(v, *p.type, line));
            }
        } else {
            s = std::make_shared<Value>(Convert(Eval(p.def), *p.type, line));
        }
        bound.push_back(s);
    }

    Frame fr;
    fr.scopes.emplace_back();
    fr.self = self;
    fr.fn = &fn;
    for (size_t k = 0; k < fn.params.size(); ++k)
        if (!fn.params[k].name.empty()) fr.scopes.back()[fn.params[k].name] = bound[k];
    frames.push_back(fr);
    Flow fl;
    try {
        fl = Exec(fn.body);
    } catch (...) {
        frames.pop_back();
        throw;
    }
    Value ret = F().ret;
    frames.pop_back();
    if (fn.ret->kind != Kind::Void && !fn.ret->isAuto && fl != Flow::Return)
        throw Err(fn.line, "hàm '" + fn.name + "' kết thúc mà chưa 'return' giá trị kiểu " + fn.ret->Name());
    return ret;
}

Flow Program::Impl::Exec(const StmtP& s) {
    Step(s->line);
    switch (s->k) {
    case SK::Empty: return Flow::Normal;
    case SK::Block: {
        F().scopes.emplace_back();
        for (const StmtP& c : s->body) {
            Flow f;
            try { f = Exec(c); } catch (...) { F().scopes.pop_back(); throw; }
            if (f != Flow::Normal) { F().scopes.pop_back(); return f; }
        }
        F().scopes.pop_back();
        return Flow::Normal;
    }
    case SK::Decl: {
        for (const Decl& d : s->decls) {
            Slot slot;
            if (s->type->isRef) {
                if (!d.init) throw Err(d.line, "tham chiếu '" + d.name + "' phải được gán ngay khi khai báo");
                slot = EvalRef(d.init);
            } else if (s->type->isAuto) {
                if (!d.init) throw Err(d.line, "biến auto '" + d.name + "' phải có giá trị khởi tạo");
                slot = std::make_shared<Value>(Copy(Eval(d.init)));
            } else {
                slot = std::make_shared<Value>(Default(*s->type));
                if (d.init) Store(slot, Eval(d.init), d.line);
                else if (s->type->kind == Kind::Int || s->type->kind == Kind::Float) {
                    // C++ để giá trị rác; ta đặt 0 nhưng lời khuyên vẫn là: luôn khởi tạo biến!
                }
            }
            if (s->type->isConst && !s->type->isRef) slot->readonly = true;
            Declare(d.name, slot, d.line);
        }
        return Flow::Normal;
    }
    case SK::ExprS: Eval(s->e1); return Flow::Normal;
    case SK::If:
        if (Eval(s->e1).AsBool()) return Exec(s->s1);
        if (s->s2) return Exec(s->s2);
        return Flow::Normal;
    case SK::While:
        while (Eval(s->e1).AsBool()) {
            Step(s->line);
            Flow f = Exec(s->s1);
            if (f == Flow::Break) break;
            if (f == Flow::Return) return f;
        }
        return Flow::Normal;
    case SK::DoWhile:
        do {
            Step(s->line);
            Flow f = Exec(s->s1);
            if (f == Flow::Break) break;
            if (f == Flow::Return) return f;
        } while (Eval(s->e1).AsBool());
        return Flow::Normal;
    case SK::For: {
        F().scopes.emplace_back();
        try {
            if (s->s1) Exec(s->s1);
            while (!s->e1 || Eval(s->e1).AsBool()) {
                Step(s->line);
                Flow f = Exec(s->s2);
                if (f == Flow::Break) break;
                if (f == Flow::Return) { F().scopes.pop_back(); return f; }
                if (s->e2) Eval(s->e2);
            }
        } catch (...) { F().scopes.pop_back(); throw; }
        F().scopes.pop_back();
        return Flow::Normal;
    }
    case SK::RangeFor: {
        Slot arr = EvalRef(s->e1);
        if (arr->kind != Kind::Vector) throw Err(s->line, "vòng for (x : ...) cần một std::vector");
        std::vector<Slot> items = arr->items;
        for (const Slot& it : items) {
            Step(s->line);
            F().scopes.emplace_back();
            Slot v = s->type->isRef ? it : std::make_shared<Value>(s->type->isAuto ? Copy(*it) : Convert(*it, *s->type, s->line));
            F().scopes.back()[s->name] = v;
            Flow f;
            try { f = Exec(s->s1); } catch (...) { F().scopes.pop_back(); throw; }
            F().scopes.pop_back();
            if (f == Flow::Break) break;
            if (f == Flow::Return) return f;
        }
        return Flow::Normal;
    }
    case SK::Switch: {
        Value v = Eval(s->e1);
        size_t start = s->body.size();
        bool found = false;
        for (const Case& c : s->cases)
            if (c.label && Eval(c.label).AsInt() == v.AsInt()) { start = c.start; found = true; break; }
        if (!found)
            for (const Case& c : s->cases) if (!c.label) { start = c.start; break; }
        F().scopes.emplace_back();
        for (size_t k = start; k < s->body.size(); ++k) {
            Flow f;
            try { f = Exec(s->body[k]); } catch (...) { F().scopes.pop_back(); throw; }
            if (f == Flow::Break) break;
            if (f != Flow::Normal) { F().scopes.pop_back(); return f; }
        }
        F().scopes.pop_back();
        return Flow::Normal;
    }
    case SK::Break: return Flow::Break;
    case SK::Continue: return Flow::Continue;
    case SK::Return: {
        const FuncDef* fn = F().fn;
        if (s->e1) {
            Value v = Eval(s->e1);
            if (fn && fn->ret->kind == Kind::Void && !fn->ret->isAuto) throw Err(s->line, "hàm void không được return giá trị");
            F().ret = (fn && !fn->ret->isAuto) ? Convert(v, *fn->ret, s->line) : Copy(v);
        } else if (fn && fn->ret->kind != Kind::Void) {
            throw Err(s->line, "thiếu giá trị sau return (hàm cần trả về " + fn->ret->Name() + ")");
        }
        return Flow::Return;
    }
    }
    return Flow::Normal;
}

// =============================================================================
//  PROGRAM (API cho game)
// =============================================================================
Program::Program() : impl_(new Impl) {
    impl_->P = this;
    impl_->vec3 = std::make_shared<StructDef>();
    impl_->vec3->name = "vec3";
    for (const char* n : { "x", "y", "z" }) impl_->vec3->fields.push_back({ MakeT(Kind::Float), n, nullptr });
}

Program::~Program() = default;

bool Program::Compile(const std::string& source, std::vector<Error>& errors) {
    errors.clear();
    structs_.clear();
    impl_->funcs.clear();
    impl_->globalDecls.clear();
    impl_->globals.clear();
    impl_->enumConsts.clear();
    impl_->enumTypes.clear();
    impl_->globalsReady = false;
    try {
        Parser ps(Lex(source), *impl_, structs_);
        ps.ParseProgram();
        impl_->frames.clear();
        impl_->steps = 0;
        impl_->EnsureGlobals();
    } catch (const Error& e) {
        errors.push_back(e);
        return false;
    }
    return true;
}

bool Program::HasFunction(const std::string& name) const { return impl_->funcs.count(name) > 0; }

const StructDef* Program::FindStruct(const std::string& name) const {
    auto it = structs_.find(name);
    return it == structs_.end() ? nullptr : it->second.get();
}

Value Program::Call(const std::string& name, const std::vector<Slot>& args) {
    auto it = impl_->funcs.find(name);
    if (it == impl_->funcs.end()) throw Error{ 0, "không tìm thấy hàm '" + name + "'" };
    impl_->steps = 0;
    impl_->frames.clear();
    impl_->frames.push_back(Frame{});
    impl_->frames.back().scopes.emplace_back();
    Value r = impl_->Invoke(*it->second, args, nullptr, nullptr, it->second->line);
    impl_->frames.clear();
    return r;
}

Value Program::CallMethod(const Slot& object, const std::string& method, const std::vector<Slot>& args) {
    if (!object || object->kind != Kind::Struct || !object->sdef) throw Error{ 0, "đối tượng không hợp lệ" };
    auto it = object->sdef->methods.find(method);
    if (it == object->sdef->methods.end()) throw Error{ 0, object->sdef->name + " không có hàm '" + method + "'" };
    impl_->steps = 0;
    impl_->frames.clear();
    impl_->frames.push_back(Frame{});
    impl_->frames.back().scopes.emplace_back();
    Value r = impl_->Invoke(*it->second, args, nullptr, object, it->second->line);
    impl_->frames.clear();
    return r;
}

Slot Program::NewObject(const std::string& structName) {
    const StructDef* sd = (structName == "vec3") ? impl_->vec3.get() : FindStruct(structName);
    if (!sd) throw Error{ 0, "không tìm thấy struct/class '" + structName + "'" };
    TypeInfo t;
    t.kind = Kind::Struct;
    t.sdef = sd;
    impl_->frames.clear();
    impl_->frames.push_back(Frame{});
    impl_->frames.back().scopes.emplace_back();
    Slot s = std::make_shared<Value>(impl_->Default(t));
    impl_->frames.clear();
    return s;
}

Slot Program::Field(const Slot& object, const std::string& field) {
    if (!object || object->kind != Kind::Struct || !object->sdef) return nullptr;
    int idx = object->sdef->FieldIndex(field);
    return idx < 0 ? nullptr : object->items[idx];
}

Slot Program::NewVector(Kind elemKind) {
    auto s = std::make_shared<Value>();
    s->kind = Kind::Vector;
    s->elem = MakeT(elemKind);
    return s;
}

} // namespace mcpp

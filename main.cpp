#include <iostream>
#include <optional>
#include <stdexcept>
#include <utility>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};
class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};
class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};
class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value;
    mutable int borrow_count = 0;      // number of active immutable borrows
    mutable bool mut_borrowed = false; // whether a mutable borrow is active

    void inc_borrow() const {
        if (mut_borrowed) throw BorrowError("immutable borrow while mut borrowed");
        ++borrow_count;
    }
    void dec_borrow() const {
        if (borrow_count > 0) --borrow_count;
    }
    void begin_borrow_mut() {
        if (mut_borrowed || borrow_count > 0) throw BorrowMutError("mutable borrow conflict");
        mut_borrowed = true;
    }
    void end_borrow_mut() {
        mut_borrowed = false;
    }

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    class Ref {
        const RefCell<T>* owner = nullptr;
    public:
        explicit Ref(const RefCell<T>* o) : owner(o) { if (owner) owner->inc_borrow(); }
        Ref() = default;
        ~Ref() { if (owner) owner->dec_borrow(); }
        const T& operator*() const { return owner->value; }
        const T* operator->() const { return &owner->value; }
        // copy
        Ref(const Ref& other) : owner(other.owner) { if (owner) owner->inc_borrow(); }
        Ref& operator=(const Ref& other) {
            if (this == &other) return *this;
            if (owner) owner->dec_borrow();
            owner = other.owner;
            if (owner) owner->inc_borrow();
            return *this;
        }
        // move
        Ref(Ref&& other) noexcept : owner(other.owner) { other.owner = nullptr; }
        Ref& operator=(Ref&& other) noexcept {
            if (this == &other) return *this;
            if (owner) owner->dec_borrow();
            owner = other.owner;
            other.owner = nullptr;
            return *this;
        }
    };

    class RefMut {
        RefCell<T>* owner = nullptr;
    public:
        explicit RefMut(RefCell<T>* o) : owner(o) { if (owner) owner->begin_borrow_mut(); }
        RefMut() = default;
        ~RefMut() { if (owner) owner->end_borrow_mut(); }
        T& operator*() { return owner->value; }
        T* operator->() { return &owner->value; }
        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;
        RefMut(RefMut&& other) noexcept : owner(other.owner) { other.owner = nullptr; }
        RefMut& operator=(RefMut&& other) noexcept {
            if (this == &other) return *this;
            if (owner) owner->end_borrow_mut();
            owner = other.owner;
            other.owner = nullptr;
            return *this;
        }
    };

    Ref borrow() const { return Ref(this); }

    std::optional<Ref> try_borrow() const {
        try {
            Ref r(this);
            return std::optional<Ref>(std::move(r));
        } catch (const BorrowError&) {
            return std::nullopt;
        }
    }

    RefMut borrow_mut() { return RefMut(this); }

    std::optional<RefMut> try_borrow_mut() {
        try {
            RefMut r(this);
            return std::optional<RefMut>(std::move(r));
        } catch (const BorrowMutError&) {
            return std::nullopt;
        }
    }

    ~RefCell() {
        if (borrow_count != 0 || mut_borrowed) {
            throw DestructionError("RefCell destructed with active borrows");
        }
    }
};

// No standalone I/O behavior required; the class is tested by OJ harness.

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // No I/O required for this problem.
    return 0;
}

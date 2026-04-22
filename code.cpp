#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
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
    mutable int shared_count{0};
    mutable bool mut_active{false};

    void inc_shared() const { ++shared_count; }
    void dec_shared() const { --shared_count; }
    void set_mut(bool v) const { mut_active = v; }

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    Ref borrow() const {
        if (mut_active) throw BorrowError("immutable borrow while mutable borrow exists");
        inc_shared();
        return Ref(this, &value);
    }

    std::optional<Ref> try_borrow() const {
        if (mut_active) return std::nullopt;
        inc_shared();
        return std::optional<Ref>(Ref(this, &value));
    }

    RefMut borrow_mut() {
        if (mut_active || shared_count > 0) throw BorrowMutError("mutable borrow while other borrows exist");
        set_mut(true);
        return RefMut(this, &value);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (mut_active || shared_count > 0) return std::nullopt;
        set_mut(true);
        return std::optional<RefMut>(RefMut(this, &value));
    }

    class Ref {
    private:
        const RefCell<T>* owner{nullptr};
        const T* ptr{nullptr};

    public:
        Ref() = default;
        ~Ref() {
            if (owner) owner->dec_shared();
        }

        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }

        Ref(const Ref& other) : owner(other.owner), ptr(other.ptr) {
            if (owner) owner->inc_shared();
        }
        Ref& operator=(const Ref& other) {
            if (this == &other) return *this;
            if (owner) owner->dec_shared();
            owner = other.owner;
            ptr = other.ptr;
            if (owner) owner->inc_shared();
            return *this;
        }

        Ref(Ref&& other) noexcept : owner(other.owner), ptr(other.ptr) {
            other.owner = nullptr;
            other.ptr = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this == &other) return *this;
            if (owner) owner->dec_shared();
            owner = other.owner;
            ptr = other.ptr;
            other.owner = nullptr;
            other.ptr = nullptr;
            return *this;
        }

    private:
        friend class RefCell<T>;
        Ref(const RefCell<T>* o, const T* p) : owner(o), ptr(p) {}
    };

    class RefMut {
    private:
        RefCell<T>* owner{nullptr};
        T* ptr{nullptr};

    public:
        RefMut() = default;
        ~RefMut() {
            if (owner) owner->set_mut(false);
        }

        T& operator*() { return *ptr; }
        T* operator->() { return ptr; }

        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        RefMut(RefMut&& other) noexcept : owner(other.owner), ptr(other.ptr) {
            other.owner = nullptr;
            other.ptr = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this == &other) return *this;
            if (owner) owner->set_mut(false);
            owner = other.owner;
            ptr = other.ptr;
            other.owner = nullptr;
            other.ptr = nullptr;
            return *this;
        }

    private:
        friend class RefCell<T>;
        RefMut(RefCell<T>* o, T* p) : owner(o), ptr(p) {}
    };

    ~RefCell() {
        if (mut_active || shared_count != 0) {
            throw DestructionError("RefCell destroyed while borrows exist");
        }
    }
};

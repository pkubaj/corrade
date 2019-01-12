#ifndef Corrade_Containers_Pointer_h
#define Corrade_Containers_Pointer_h
/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Corrade::Containers::Pointer, function @ref Corrade::Containers::pointer()
 */

#include <type_traits>
#include <utility> /* std::forward() */

#include "Corrade/Containers/Tags.h"
#include "Corrade/Utility/Assert.h"
#include "Corrade/Utility/Debug.h"

namespace Corrade { namespace Containers {

/**
@brief Lightweight unique pointer

Equivalent to @ref std::unique_ptr from C++11, provides an owning move-only
wrapper over a pointer of type @p T, calling @cpp delete @ce on it on
destruction. The @ref pointer() convenience function also provides an
equivalent for C++14 @cpp std::make_unique() @ce, but on C++11 as well. Can be
also thought of as a heap-allocated counterpart to @ref Optional.

Unlike @ref std::unique_ptr, this class does not provide custom deleters,
doesn't work with arrays and doesn't have a @cpp constexpr @ce API. On the
other hand that makes it fairly simple and lightweight. If you need a custom
deleter, use either @ref ScopedExit or the standard @ref std::unique_ptr. For
owning array wrappers use @ref Array, which maintains a size information and
also supports custom deleters.
@see @ref pointer(T*), @ref pointer(Args&&... args)
*/
template<class T> class Pointer {
    static_assert(!std::is_array<T>::value, "use Containers::Array for arrays instead");

    public:
        /**
         * @brief Default constructor
         *
         * Creates a @cpp nullptr @ce unique pointer.
         * @see @ref operator bool(), @ref reset()
         */
        /*implicit*/ Pointer(std::nullptr_t = nullptr) noexcept: _pointer{} {}

        /**
         * @brief Construct a pointer by value
         *
         * Takes ownership of the passed pointer.
         * @see @ref operator bool(), @ref operator->()
         */
        explicit Pointer(T* pointer) noexcept: _pointer{pointer} {}

        /**
         * @brief Construct a pointer in-place
         *
         * Allocates a new object by passing @p args to its constructor.
         * @see @ref operator bool(), @ref operator->()
         */
        template<class ...Args> explicit Pointer(InPlaceInitT, Args&&... args): _pointer{new T{std::forward<Args>(args)...}} {}

        /** @brief Copying is not allowed */
        Pointer(const Pointer<T>&) = delete;

        /** @brief Move constructor */
        Pointer(Pointer<T>&& other) noexcept: _pointer{other._pointer} {
            other._pointer = nullptr;
        }

        /** @brief Copying is not allowed */
        Pointer<T>& operator=(const Pointer<T>&) = delete;

        /** @brief Move assignment */
        Pointer<T>& operator=(Pointer<T>&& other) noexcept {
            std::swap(_pointer, other._pointer);
            return *this;
        }

        /**
         * @brief Equality comparison to a null pointer
         *
         * Returns @cpp true @ce if the poiner is @cpp nullptr @ce,
         * @cpp false @ce otherwise.
         * @see @ref operator bool()
         */
        bool operator==(std::nullptr_t) const { return !_pointer; }

        /**
         * @brief Non-equality comparison to a null pointer
         *
         * Returns @cpp false @ce if the pointer is @cpp nullptr @ce,
         * @cpp false @ce otherwise.
         * @see @ref operator bool()
         */
        bool operator!=(std::nullptr_t) const { return _pointer; }

        /**
         * @brief Destructor
         *
         * Calls @cpp delete @ce on the stored pointer.
         */
        ~Pointer() { delete _pointer; }

        /**
         * @brief Whether the pointer is non-null
         *
         * Returns @cpp false @ce if stored pointer is @cpp nullptr @ce,
         * @cpp true @ce otherwise.
         */
        explicit operator bool() const { return _pointer; }

        /**
         * @brief Underlying pointer value
         *
         * @see @ref operator bool(), @ref operator->(), @ref release()
         */
        T* get() { return _pointer; }
        const T* get() const { return _pointer; } /**< @overload */

        /**
         * @brief Access the underlying pointer
         *
         * Expects that the pointer is not @cpp nullptr @ce.
         * @see @ref operator bool(), @ref get(), @ref operator*(),
         *      @ref release()
         */
        T* operator->() {
            CORRADE_ASSERT(_pointer, "Containers::Pointer: the pointer is null", nullptr);
            return _pointer;
        }

        /** @overload */
        const T* operator->() const {
            CORRADE_ASSERT(_pointer, "Containers::Pointer: the pointer is null", nullptr);
            return _pointer;
        }

        /**
         * @brief Access the underlying pointer
         *
         * Expects that the pointer is not @cpp nullptr @ce.
         * @see @ref operator bool(), @ref get(), @ref operator->(),
         *      @ref release()
         */
        T& operator*() {
            CORRADE_ASSERT(_pointer, "Containers::Pointer: the pointer is null", *_pointer);
            return *_pointer;
        }

        /** @overload */
        const T& operator*() const {
            CORRADE_ASSERT(_pointer, "Containers::Pointer: the pointer is null", *_pointer);
            return *_pointer;
        }

        /**
         * @brief Reset the pointer to a new value
         *
         * Calls @cpp delete @ce on the previously stored pointer and replaces
         * it with @p pointer.
         * @see @ref release()
         */
        void reset(T* pointer) {
            delete _pointer;
            _pointer = pointer;
        }

        /**
         * @brief Emplace a new value
         *
         * Calls @cpp delete @ce on the previously stored pointer and allocates
         * a new object by passing @p args to its constructor.
         */
        template<class ...Args> T& emplace(Args&&... args) {
            delete _pointer;
            _pointer = new T{std::forward<Args>(args)...};
            return *_pointer;
        }

        /**
         * @brief Release the pointer ownership
         *
         * Resets the stored pointer to @cpp nullptr @ce, returning the
         * previous value.
         * @see @ref get(), @ref reset()
         */
        T* release() {
            T* const out = _pointer;
            _pointer = nullptr;
            return out;
        }

    private:
        T* _pointer;
};

/** @relates Pointer
@brief Equality comparison of a null pointer and an unique pointer

See @ref Pointer::operator==(std::nullptr_t) const for more information.
*/
template<class T> bool operator==(std::nullptr_t, const Pointer<T>& b) { return b == nullptr; }

/** @relates Pointer
@brief Non-euality comparison of a null pointer and an unique pointer

See @ref Pointer::operator!=(std::nullptr_t) const for more information.
*/
template<class T> bool operator!=(std::nullptr_t, const Pointer<T>& b) { return b != nullptr; }

/** @relatesalso Pointer
@brief Make a unique pointer

Convenience alternative to @ref Pointer::Pointer(T*). The following two
lines are equivalent:

@snippet Containers.cpp pointer

@attention Note that for types that are constructible from their own pointer
    the call would get ambiguous between this function and
    @ref pointer(Args&&... args). Such case is forbidden at compile time in
    order to prevent potentially dangerous behavior and you need to explicitly
    use the @ref Pointer constructor instead.

@see @ref pointer(Args&&... args), @ref optional(T&&)
*/
template<class T> inline Pointer<T> pointer(T* pointer) {
    static_assert(!std::is_constructible<T, T*>::value, "the type is constructible from its own pointer, which is ambiguous -- explicitly use the constructor instead");
    return Pointer<T>{pointer};
}

namespace Implementation {
    template<class T, class ...Args> struct IsFirstAPointer: std::false_type {};
    template<class T> struct IsFirstAPointer<T, T*>: std::true_type {};
}

/** @relatesalso Pointer
@brief Make a unique pointer

Convenience alternative to @ref Pointer::Pointer(InPlaceInitT, Args&&... args).
The following two lines are equivalent:

@snippet Containers.cpp pointer-inplace

@attention Note that for types that are constructible from their own pointer
    the call would get ambiguous between this function and @ref pointer(T*).
    Such case is forbidden at compile time in order to prevent potentially
    dangerous behavior and you need to explicitly use the @ref Pointer
    constructor instead.

@see @ref pointer(T*), @ref optional(Args&&... args)
*/
template<class T, class ...Args> inline Pointer<T> pointer(Args&&... args) {
    static_assert(!Implementation::IsFirstAPointer<T, Args...>::value || !std::is_constructible<T, T*>::value, "attempt to construct a type from its own pointer, which is ambiguous --  explicitly use the constructor instead");
    return Pointer<T>{InPlaceInit, std::forward<Args>(args)...};
}

/** @debugoperator{Pointer} */
template<class T> Utility::Debug& operator<<(Utility::Debug& debug, const Pointer<T>& value) {
    return debug << value.get();
}

}}

#endif
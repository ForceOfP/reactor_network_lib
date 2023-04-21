#pragma once
#ifndef Function_Wrapper_Marco
#define Function_Wrapper_Marco

#include <algorithm>
#include <memory>
#include <concepts>

class function_wrapper {
private:
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() = default;    
    };
    std::unique_ptr<impl_base> impl_;
    template<std::invocable F>
    struct impl_type: impl_base {
        F f_;
        impl_type(F&& f): f_(std::move(f)) {}
        void call() override { f_(); }
    };
public:
    template<std::invocable F> 
    function_wrapper(F&& f): impl_(new impl_type<F>(std::move(f))) {}
    void operator() () { impl_->call(); }
    function_wrapper() = default;
    function_wrapper(function_wrapper&& src) noexcept : impl_(std::move(src.impl_)) {}
    function_wrapper& operator= (function_wrapper&& rhs)  noexcept {
        impl_ = std::move(rhs.impl_);
        return *this;
    }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator= (const function_wrapper&) = delete;
};

#endif
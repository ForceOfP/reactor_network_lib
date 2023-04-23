#pragma once
#ifndef Function_Wrapper_Marco
#define Function_Wrapper_Marco

#include <algorithm>
#include <memory>
#include <concepts>

class FunctionWrapper {
private:
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() {}     
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
    FunctionWrapper(F&& f): impl_(new impl_type<F>(std::move(f))) {}
    void operator() () { impl_->call(); }
    FunctionWrapper() = default;
    FunctionWrapper(FunctionWrapper&& src): impl_(std::move(src.impl_)) {}
    FunctionWrapper& operator= (FunctionWrapper&& rhs) {
        impl_ = std::move(rhs.impl_);
        return *this;
    }
    FunctionWrapper(const FunctionWrapper&) = delete;
    FunctionWrapper(FunctionWrapper&) = delete;
    FunctionWrapper& operator= (const FunctionWrapper&) = delete;
};

#endif
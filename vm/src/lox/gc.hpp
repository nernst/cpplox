#pragma once

#include "common.hpp"
#include "value.hpp"
#include <atomic>
#include <iosfwd>

namespace lox
{
    class GC;

    enum class Tracking{
        STRONG,
        WEAK,
        HELD,
    };


    class Trackable
    {
    public:
        explicit Trackable(Tracking tracking = Tracking::STRONG);
        virtual ~Trackable() noexcept;

    protected:
        static bool is_used(Object const* object);
        void mark_objects(GC& gc);

    private:
        friend class GC;
        virtual void do_mark_objects(GC& gc) = 0;
        virtual void remove_weak_refs(GC&) { }

    };

    template<typename T>
    class gcroot : public Trackable
    {
    public:
        gcroot() = default;

        explicit gcroot(T* object)
        : Trackable{}
        , object_{object}
        { }

        gcroot(gcroot const& copy)
        : Trackable{}
        , object_{copy.object_}
        { }

        gcroot(gcroot&& other)
        : Trackable{}
        , object_{std::exchange(other.object_, nullptr)}
        { }

        ~gcroot() noexcept {}

        gcroot& operator=(gcroot const& copy)
        {
            object_ = copy.object_;
            return *this;
        }

        gcroot& operator=(gcroot&& other)
        {
            object_ = std::exchange(other.object_, nullptr);
            return *this;
        }

        T* operator->() { return object_; }
        T const* operator->() const { return object_; }
        T* get() { return object_; }
        T const* get() const { return object_; }
        T* release() { return std::exchange(object_, nullptr); }

        explicit operator bool() const { return object_ != nullptr; }
        bool operator!() const { return object_ == nullptr; }

    private:
        T* object_ = nullptr;

        void do_mark_objects(GC& gc) override;
    };

    class GC
    {
    public:
        GC(GC const&) = delete;
        GC(GC&&) = default;

        GC& operator=(GC const&) = delete;
        GC& operator=(GC&&) = default;

        void collect();

        static GC& instance();

        void mark_value(Value& value);
        void mark_object(Object* object);

    private:
        GC();
        std::atomic<Object*> root_;
        std::ostream& stderr_;
        std::vector<std::tuple<Trackable*, Tracking>> tracking_;
        std::vector<Object*> gray_stack_;
        bool running_ = false;

        void track(Trackable* trackable, Tracking tracking);
        bool untrack(Trackable* trackable);

        void free_objects();
        void free_object(Object*);
        void mark_roots();
        void remove_weak_refs();
        void trace_references();
        void sweep();

        friend class Object;
        friend class Trackable;
    };

    template<class T>
    inline void gcroot<T>::do_mark_objects(GC& gc)
    {
        if (object_)
        {
            gc.mark_object(object_);
        }
    }

}

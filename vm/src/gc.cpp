#include "lox/gc.hpp"
#include "lox/vm.hpp"
#include "lox/object.hpp"
#include <fmt/ostream.h>

namespace lox {

    Trackable::Trackable(Tracking tracking)
    {
        if (tracking != Tracking::HELD)
        {
            GC::instance().track(this, tracking);
        }
    }

    Trackable::~Trackable()
    {
        if (GC::instance().untrack(this))
        {
            // only force collection if this instance was tracked
            GC::instance().collect();
        }
    }

    bool Trackable::is_used(Object const* object)
    {
        if (object == nullptr)
            return false;
        return object->gc_marked_;
    }

    void Trackable::mark_objects(GC& gc)
    {
        #ifdef DEBUG_LOG_GC
        fmt::print(
            std::cerr,
            "Trackable::mark_objects {} {} begin\n",
            static_cast<void*>(this),
            typeid(*this).name());
        #endif

        do_mark_objects(gc);

        #ifdef DEBUG_LOG_GC
        fmt::print(
            std::cerr,
            "Trackable::mark_objects {} end\n",
            static_cast<void*>(this));
        #endif
    }

    GC::GC()
    : root_{nullptr}
    , stderr_{std::cerr}
    { }

    GC& GC::instance()
    {
        static GC gc;
        return gc;
    }

    void GC::track(Trackable* trackable, Tracking tracking)
    {
        assert(trackable);

        #ifdef DEBUG_LOG_GC
        fmt::print(
            stderr_, 
            "-- gc tracking: {} {}\n",
            static_cast<void*>(trackable),
            typeid(*trackable).name()
        );
        #endif

        auto iter = std::find_if(
            std::begin(tracking_),
            std::end(tracking_),
            [=](auto const& value) { return std::get<0>(value) == trackable; }
        );

        assert(iter == std::end(tracking_));

        tracking_.emplace_back(trackable, tracking);
    }

    bool GC::untrack(Trackable* trackable)
    {
        assert(trackable);

        #ifdef DEBUG_LOG_GC
        fmt::print(
            stderr_, 
            "-- gc untracking: {} {}\n",
            static_cast<void*>(trackable),
            typeid(*trackable).name()
        );
        #endif

        auto iter = std::find_if(
            std::begin(tracking_),
            std::end(tracking_),
            [=](auto const& value) { return std::get<0>(value) == trackable; }
        );

        if (iter != std::end(tracking_))
        {
            tracking_.erase(iter);
            return true;
        }
        return false;
    }

    void GC::collect()
    {
        if (running_)
            return;

        running_ = true;

    #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- gc begin\n");
    #endif

        mark_roots();
        trace_references();
        remove_weak_refs();
        sweep();

    #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- gc end\n");
    #endif
        running_ = false;
    }

    void GC::free_objects()
    {
        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- gc free_objects() begin\n");
        #endif

        Object* root{nullptr};
        root = root_.exchange(root);

        while (root)
        {
            Object* next = root->gc_next_;
            free_object(root);
            root = next;
        }

        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- gc free_objects() end\n");
        #endif
    }

    void GC::free_object(Object* object)
    {
    #ifdef DEBUG_LOG_GC
        fmt::print(
            stderr_, 
            "{} free type {} - ",
            static_cast<void*>(object),
            object->type_name()
            );
        print_object(stderr_, *object);
        fmt::print(stderr_, "\n");
    #endif
        delete object;
    }

    void GC::mark_roots()
    {
        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- mark_roots begin\n");
        #endif

        for (auto [tracked, tracking] : tracking_)
        {
            if (tracking == Tracking::WEAK)
                continue;

            #ifdef DEBUG_LOG_GC
            fmt::print(
                stderr_,
                "--   root: {} {}\n",
                static_cast<void*>(tracked),
                typeid(*tracked).name()
            );
            #endif
            tracked->mark_objects(*this);
        }

        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- mark_roots end\n");
        #endif
    }

    void GC::mark_value(Value& value)
    {
        Object* obj;
        if (value.try_get(obj) && obj)
            mark_object(obj);
    }

    void GC::mark_object(Object* object)
    {
        if (object == nullptr)
            return;
        if (object->gc_marked_)
            return;

        #ifdef DEBUG_LOG_GC
        fmt::print(
            stderr_,
            "\t{} mark ",
            static_cast<void*>(object)
        );
        print_object(stderr_, *object);
        stderr_ << "\n";
        #endif

        object->gc_marked_ = true;
        gray_stack_.push_back(object);
    }

    void GC::trace_references()
    {
        while (!gray_stack_.empty())
        {
            Object* object = gray_stack_.back();
            gray_stack_.pop_back();
            object->gc_blacken(*this);
        }
    }

    void GC::sweep()
    {
        Object* previous{nullptr};
        Object* object{root_.load()};

        while (object)
        {
            if (object->gc_marked_)
            {
                object->gc_marked_ = false;
                previous = object;
                object = object->gc_next_;
            }
            else
            {
                Object* unreached{object};
                object = object->gc_next_;

                if (previous != nullptr)
                {
                    previous->gc_next_ = object;
                }
                else
                {
                    root_ = object;
                }

                free_object(unreached);
            }
        }
    }

    void GC::remove_weak_refs()
    {
        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- remove_weak_refs {} begin\n", static_cast<void*>(this));
        #endif

        for (auto [tracked, tracking] : tracking_)
        {
            if (tracking != Tracking::WEAK)
                continue;

            tracked->remove_weak_refs(*this);
        }

        #ifdef DEBUG_LOG_GC
        fmt::print(stderr_, "-- remove_weak_refs {} end\n", static_cast<void*>(this));
        #endif
    }

}

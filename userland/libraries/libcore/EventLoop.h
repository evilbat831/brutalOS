/*
 * Copyright (c) 2021, Krisna Pranav
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// includes
#include <base/Forward.h>
#include <base/Function.h>
#include <base/HashMap.h>
#include <base/Noncopyable.h>
#include <base/NonnullOwnPtr.h>
#include <base/Vector.h>
#include <base/WeakPtr.h>
#include <libcore/Forward.h>
#include <sys/time.h>
#include <sys/types.h>

namespace Core {

class EventLoop {
public:
    enum class MakeInspectable {
        No,
        Yes,
    };

    explicit EventLoop(MakeInspectable = MakeInspectable::No);
    ~EventLoop();

    int exec();

    enum class WaitMode {
        WaitForEvents,
        PollForEvents,
    };

    void pump(WaitMode = WaitMode::WaitForEvents);

    void post_event(Object& receiver, NonnullOwnPtr<Event>&&);

    static EventLoop& main();
    static EventLoop& current();

    bool was_exit_requested() const { return m_exit_requested; }

    static int register_timer(Object&, int milliseconds, bool should_reload, TimerShouldFireWhenNotVisible);
    static bool unregister_timer(int timer_id);

    static void register_notifier(Badge<Notifier>, Notifier&);
    static void unregister_notifier(Badge<Notifier>, Notifier&);

    void quit(int);
    void unquit();

    void take_pending_events_from(EventLoop& other)
    {
        m_queued_events.extend(move(other.m_queued_events));
    }

    static void wake();

    static int register_signal(int signo, Function<void(int)> handler);
    static void unregister_signal(int handler_id);

    enum class ForkEvent {
        Child,
    };
    static void notify_forked(ForkEvent);

private:
    void wait_for_event(WaitMode);
    Optional<struct timeval> get_next_timer_expiration();
    static void dispatch_signal(int);
    static void handle_signal(int);

    struct QueuedEvent {
        BASE_MAKE_NONCOPYABLE(QueuedEvent);

    public:
        QueuedEvent(Object& receiver, NonnullOwnPtr<Event>);
        QueuedEvent(QueuedEvent&&);
        ~QueuedEvent();

        WeakPtr<Object> receiver;
        NonnullOwnPtr<Event> event;
    };

    Vector<QueuedEvent, 64> m_queued_events;
    static pid_t s_pid;

    bool m_exit_requested { false };
    int m_exit_code { 0 };

    static int s_wake_pipe_fds[2];

    struct Private;
    NonnullOwnPtr<Private> m_private;
};

}

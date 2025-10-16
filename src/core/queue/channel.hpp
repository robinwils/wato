#pragma once

#include <bx/spscqueue.h>

#include <memory>

#include "core/sys/log.hpp"

template <typename _MsgT>
class Channel
{
   public:
    using queue_type = bx::SpScUnboundedQueueT<_MsgT>;

    Channel() : mQueue(&mAlloc) {}
    Channel(const Channel&)            = default;
    Channel(Channel&&)                 = default;
    Channel& operator=(const Channel&) = default;
    Channel& operator=(Channel&&)      = default;
    ~Channel()                         = default;

    void                   Send(_MsgT* aPkt) { mQueue.push(aPkt); }
    std::shared_ptr<_MsgT> Recv() { return std::shared_ptr<_MsgT>(mQueue.pop()); }

    template <typename Func>
    void Drain(Func&& aHandler)
    {
        while (std::shared_ptr<_MsgT> ev = Recv()) {
            aHandler(ev);
        }
    }

   private:
    bx::DefaultAllocator mAlloc;
    queue_type           mQueue;
};

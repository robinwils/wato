#pragma once

#include <bx/spscqueue.h>

#include <memory>

template <typename _In, typename _Out>
class Channel
{
   public:
    using inbound_queue  = bx::SpScUnboundedQueueT<_In>;
    using outbound_queue = bx::SpScUnboundedQueueT<_Out>;

    Channel() : mRead(&mAlloc), mWrite(&mAlloc) {}
    Channel(const Channel&)            = default;
    Channel(Channel&&)                 = default;
    Channel& operator=(const Channel&) = default;
    Channel& operator=(Channel&&)      = default;
    ~Channel()                         = default;

    void Send(_Out* aPkt) { mWrite.push(aPkt); }

    std::unique_ptr<_In> Recv() { return std::unique_ptr<_In>(std::move(mRead.pop())); }

   private:
    bx::DefaultAllocator mAlloc;
    inbound_queue        mRead;
    outbound_queue       mWrite;
};

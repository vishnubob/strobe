#ifndef __RING_BUFFER_T__
#define __RING_BUFFER_T__

#include "defines.h"

template <class T>
class RingBuffer
{
public:
    RingBuffer()
    {
        _capacity = BUFFER_SIZE;
        _buffer_end = _buffer + _capacity;
        _head = _buffer;
        _tail = _head;
    }

    bool is_full()
    {
        uint32 hd = (_head - _buffer);
        uint32 tl = (_tail - _buffer);
        return ((hd + 1) % _capacity) == tl;
    }

    bool is_empty()
    {
        return (_head == _tail);
    }

    bool push_back(const T item)
    {
        /* spinwait */
        while (is_full()) {}
        *_head = item;
        _head += 1;
        if (_head >= _buffer_end)
            _head = _buffer;
        return true;
    }

    T* pop_front()
    {
        if (is_empty())
            return NULL;
        T* ret = const_cast<T*>(_tail);
        _tail += 1;
        if(_tail >= _buffer_end)
            _tail = _buffer;
        return ret;
    }

private:
    size_t                  _capacity;
    T                       _buffer[BUFFER_SIZE];
    T                       *_buffer_end;
    T volatile              *_head;
    T volatile              *_tail;
};

#endif // __RING_BUFFER_T__

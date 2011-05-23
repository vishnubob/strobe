#ifndef __RING_BUFFER_T__
#define __RING_BUFFER_T__

#include "defines.h"

template <class T>
class Buffer
{
public:
    Buffer()
    {
        _capacity = BUFFER_SIZE;
        _buffer_end = _buffer + _capacity;
        reset();
    }

    void reset()
    {
        _wtr = _buffer;
        _rdr = _wtr;
    }

    bool is_full()
    {
        return (_wtr == _buffer_end);
    }

    bool is_empty()
    {
        return (_rdr == _wtr);
    }

    bool push_back(const T item)
    {
        if (is_full())
            return false;
        *_wtr = item;
        _wtr += 1;
        return true;
    }

    T* pop_front()
    {
        if (is_empty())
            return NULL;
        T* ret = _rdr;
        _rdr += 1;
        return ret;
    }

private:
    size_t                  _capacity;
    T                       _buffer[BUFFER_SIZE];
    T                       *_wtr;
    T                       *_rdr;
    T                       *_buffer_end;
};

template <class T>
class DoubleBuffer
{
public:
    DoubleBuffer() 
        : _page_count(0)
    {
        _write_buffer = &(_buffer_1);
        _read_buffer = &(_buffer_2);
    }

    bool push_back(const T item)
    {
        if (_write_buffer->is_full())
            wait_for_page_flip();
        return _write_buffer->push_back(item);
    }

    T* pop_front()
    {
        if (_read_buffer->is_empty())
            page_flip();
        return _read_buffer->pop_front();
    }

    void page_flip()
    {
        Buffer<T> *_tmp_buffer;
        _tmp_buffer = _write_buffer;
        _write_buffer = _read_buffer;
        _read_buffer = _tmp_buffer;
        _write_buffer->reset();
        _page_count++;
    }

    void wait_for_page_flip()
    {
        register uint32 cur_page_cnt = _page_count;
        //SerialUSB.println("W");
        while (cur_page_cnt == _page_count)
        {}
    }

private:
    Buffer<T>               _buffer_1;
    Buffer<T>               _buffer_2;
    volatile uint32         _page_count;
    Buffer<T>               *_read_buffer;
    Buffer<T>               *_write_buffer;
};

#endif // __RING_BUFFER_T__

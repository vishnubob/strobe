#ifndef __RING_BUFFER_T__
#define __RING_BUFFER_T__

template <class T>
class RingBuffer
{
public:
    void init(size_t cap)
    {
        _size = 0;
        _capacity = cap;
        //_buffer = (T*)malloc(sizeof(T) * _capacity);
        _buffer_end = _buffer + _capacity;
        _head = _buffer;
        _tail = _head;
    }

    bool push_back(const T item)
    {
        if (_capacity == _size)
            return false;
        *_head = item;
        _head += 1;
        if (_head >= _buffer_end)
            _head = _buffer;
        _size += 1;
        return true;
    }

    T* pop_front()
    {
        if (_size == 0)
            return NULL;
        T* ret = _tail;
        _tail += 1;
        if(_tail >= _buffer_end)
            _tail = _buffer;
        _size -= 1;
        return ret;
    }

private:
    size_t      _size;
    size_t      _capacity;
    T           _buffer[BUFFER_SIZE];
    T           *_buffer_end;
    T           *_head;
    T           *_tail;
};

#endif // __RING_BUFFER_T__

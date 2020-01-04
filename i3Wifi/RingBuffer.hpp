#pragma once
#include <cstdint>

template <size_t N>
class RingBuffer
{
public:
    size_t size() const { return count; }
    bool empty() const { return head == tail; }

    bool push(uint8_t c, size_t* pos)
    {
        if (bcapacity() < 2)
            return false;
        ++count;
        bpush(1);
        *pos = tail;
        bpush(c);
        return true;
    }
    bool append(uint8_t c, size_t pos)
    {
        if (bcapacity() < 2)
            return false;

        uint8_t& length = buffer[(pos - 1) % N];
        ++length;
        bpush(c);
        return true;
    }

    /*
    bool push(const uint8_t* line, uint8_t length, size_t* pos = nullptr)
    {
        if (bcapacity() < length+1)
            return false;

        ++count;
        bpush(length);
        if (pos) *pos = tail;
        for (unsigned i = 0; i < length; ++i)
            bpush(line[i]);
        return true;
    }

    uint8_t get(uint8_t* line) const { return get(head+1, line); }
    uint8_t get(size_t pos, uint8_t* line) const
    {
        uint8_t length = peek(pos - 1);
        for (unsigned i = 0; i < length; ++i, ++pos)
            line[i] = buffer[pos % N];

        return length;
    }
    */

    template <typename Fn>
    void emit(Fn&& outc) const { emit(head+1, outc); }
    template <typename Fn>
    void emit(size_t pos, Fn&& outc) const
    {
        uint8_t length = peek(pos - 1);
        for (unsigned i = 0; i < length; ++i, ++pos)
            outc(buffer[pos % N]);
    }

    bool pop()
    {
        if (empty())
            return false;
        --count;
        uint8_t length = peek(head);
        head += (length + 1);
        head = head % N;
        return true;
    }

private:
    size_t bsize() const
    {
        if (head == tail)
            return 0;
        if (tail > head)
            return tail - head;
        else
            return N - (head - tail);
    }

    size_t bcapacity() const { return (N - bsize()) -1; }
    uint8_t peek(size_t pos) const { return buffer[pos % N]; }

    void bpush(uint8_t c)
    {
        buffer[tail] = c;
        ++tail;
        tail = tail % N;
    }

private:
    uint8_t buffer[N];
    size_t head = 0;
    size_t tail = 0;
    size_t count = 0;
};

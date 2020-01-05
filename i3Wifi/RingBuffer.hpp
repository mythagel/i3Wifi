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

    template <typename Fn>
    void emit(Fn&& outc) const { emit(readhead+1, outc); }
    bool pop_read()
    {
        if (empty())
            return false;
        --count;
        uint8_t length = peek(readhead);
        readhead += (length + 1);
        readhead = readhead % N;
        return true;
    }
    void reset_read() { readhead = head; }
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
    size_t readhead = 0;
};

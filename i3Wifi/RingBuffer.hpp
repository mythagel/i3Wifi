/* 
 * Copyright (C) 2020  Nicholas Gill
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <cstdint>

template <size_t N>
class RingBuffer
{
public:
    size_t size() const { return count; }
    bool empty() const { return count == 0 && head == tail; }

    bool append(uint8_t c)
    {
        if (bcapacity() < 2)
            return false;

        if (write == (size_t)-1)
        {
            ++count;
            write = tail;
            bpush(0);
        }

        uint8_t& length = buffer[write];
        if (length == 255)
            return false;
        ++length;
        bpush(c);
        return true;
    }
    bool push()
    {
        if (write == (size_t)-1)
            return false;
        write = (size_t)-1;
        return true;
    }

    template <typename Fn>
    void emit(Fn&& outc) const { emit(readhead, outc); }
    bool pop_read()
    {
        if (readhead == tail || readhead == write)
            return false;
        uint8_t length = peek(readhead);
        readhead += (length + 1);
        readhead = readhead % N;
        return true;
    }
    void reset_read() { readhead = head; }
    template <typename Fn>
    void emit(size_t pos, Fn&& outc) const
    {
        if (readhead == tail || readhead == write)
            return;
        uint8_t length = peek(pos++);
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
        if (empty())
            return 0;
        if (tail > head)
            return tail - head;
        else
            return N - (head - tail);
    }

    size_t bcapacity() const { return (N - bsize()); }
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
    size_t write = -1;
};

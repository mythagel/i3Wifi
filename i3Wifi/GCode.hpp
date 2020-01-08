#include <cstdint>
#include "RingBuffer.hpp"

namespace GCode
{

class BufferLine
{
public:
    using Callback = void (void* context);
    BufferLine(Callback* update, void* context): update(update), context(context) {}

    void process(uint8_t c)
    {
        while (! rb.append(c))
            update(context);

        if (c == '\n')
        {
            rb.push();
            update(context);
        }
    }

    RingBuffer<256>& buffer() { return rb; }

private:
    Callback* update;
    void* context;
    RingBuffer<256> rb;
};

class Checksum
{
public:
    Checksum(BufferLine& bufferLine) : bufferLine(bufferLine) {}
    void process(uint8_t c)
    {
        constexpr long kMaxLineNumber = 9;  // Minimises transmission length
        if (lineStart && c != '\n')
        {
            if (lineNumber+1 > kMaxLineNumber)
            {
                lineNumber = -1;
                process('M');process('1');process('1');process('0');process('\n');  // M110
            }

            ++lineNumber;
            emit('N');
            checksum = checksum ^ 'N';

            emitNumber(lineNumber);
            lineStart = false;
        }

        if (c == '\n')
        {
            uint8_t cs = checksum & 0xFF;
            emit('*');
            emitNumber(cs);
            emit(c);

            lineStart = true;
            checksum = 0;
        }
        else
        {
            checksum = checksum ^ c;
            emit(c);
        }
    }

private:
    void emitNumber(unsigned n)
    {
        char buffer[16];
        char* end = buffer+16;
        char* ptr = end;

        if (n == 0)
        {
            emit('0');
            checksum = checksum ^ '0';
            return;
        }
        while (n)
        {
            *--ptr = '0' + (n % 10);
            n /= 10;
        }
        while (ptr != end)
        {
            emit(*ptr);
            checksum = checksum ^ *ptr;
            ++ptr;
        }
    }

    void emit(uint8_t c)
    {
        bufferLine.process(c);
    }

private:
    BufferLine& bufferLine;
    bool lineStart = true;
    long lineNumber = 0;
    int checksum = 0;
};

class MinDigits
{
public:
    MinDigits(Checksum& checksum) : checksum(checksum) {}
    void process(uint8_t c)
    {
        if (in_mantissa)
        {
            if (c == '0')
                ++zero_digits;
            else if (isdigit(c))
                flush(c);
            else
            {
                reset(c);
                in_mantissa = false;
            }
        }
        else
        {
            if (c == '.')
            {
                in_mantissa = true;
                emit_point = true;
            }
            else
                emit(c);
        }
    }

private:
    static bool isdigit(uint8_t c)
    {
        return c >= '0' && c <= '9';
    }
    void flush(uint8_t c)
    {
        if (emit_point)
            emit('.');
        emit_point = false;
        while (zero_digits)
        {
            emit('0');
            --zero_digits;
        }
        reset(c);
    }
    void reset(uint8_t c)
    {
        zero_digits = 0;
        emit(c);
    }
    void emit(uint8_t c)
    {
        checksum.process(c);
    }

private:
    Checksum& checksum;
    bool in_mantissa = false;
    bool emit_point = false;
    unsigned zero_digits = 0;
};

class Minify
{
public:
    Minify(MinDigits& digits) : digits(digits) {}
    void process(uint8_t* buf, size_t bufLen)
    {
        size_t readIndex = 0;
        while (readIndex < bufLen)
        {
            uint8_t c = buf[readIndex];
            switch(state)
            {
                case State::Emit:
                    ++readIndex;
                    switch (c)
                    {
                        case '\r':
                        case ' ':
                            state = State::Whitespace;
                            break;
                        case ';':
                            state = State::Comment;
                            break;
                        case '\n':
                            if (lastChar == -1 || lastChar == '\n')
                                break;
                            // fallthrough
                        default:
                            emit(c);
                            lastChar = c;
                            break;
                    }
                    break;
                case State::Whitespace:
                    if (c == ' ' || c == '\t' || c == '\r')
                        ++readIndex;
                    else
                        state = State::Emit;
                    break;
                case State::Comment:
                    if (c == '\r' || c == '\n')
                        state = State::Emit;
                    else
                        ++readIndex;            // consume
                    break;
            }
        }
    }

private:
    void emit(uint8_t c)
    {
        digits.process(c);
    }

private:
    enum class State
    {
        Emit,
        Whitespace,
        Comment,
    };

    MinDigits& digits;
    State state = State::Emit;
    int lastChar = -1;
};

}

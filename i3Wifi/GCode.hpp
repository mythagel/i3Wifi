#include <cstdint>
#include "RingBuffer.hpp"

namespace GCode
{

class BufferLine
{
public:
    using Callback = void (void* context);
    BufferLine(Callback* callback, void* context): callback(callback), context(context) {}

    void process(uint8_t c)
    {
        while (true)
        {
            bool success;
            if (pos == (size_t)-1)
                success = rb.push(c, &pos);
            else
                success = rb.append(c, pos);
            if (success)
                break;

            callback(context);
        }

        if (c == '\n')
        {
            callback(context);
            pos = -1;
        }
    }

    template <typename Fn> void emit(Fn&& outc) const { rb.emit(outc); }
    template <typename Fn> void emit(size_t pos, Fn&& outc) const { rb.emit(pos, outc); }
    bool pop() { return rb.pop(); }

private:
    Callback* callback;
    void* context;
    RingBuffer<256> rb;
    size_t pos = -1;
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

class Minify
{
public:
    Minify(Checksum& checksum) : checksum(checksum) {}
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
        checksum.process(c);
    }

private:
    enum class State
    {
        Emit,
        Whitespace,
        Comment,
    };

    Checksum& checksum;
    State state = State::Emit;
    int lastChar = -1;
};

}

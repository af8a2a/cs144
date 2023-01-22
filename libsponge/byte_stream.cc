#include "byte_stream.hh"

#include <algorithm>
#include <cstddef>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : capacity_(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t size = capacity_ - buffer_size();
    size = min(data.size(), size);
    write_count_ += size;
    for (size_t i = 0; i < size; i++) {
        buffer_.push_back(data[i]);
    }
    return size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = len;
    if (length > buffer_.size()) {
        length = buffer_.size();
    }
    return string().assign(buffer_.begin(), buffer_.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t length = len;
    if (length > buffer_.size()) {
        length = buffer_.size();
    }
    read_count_ += length;
    while (length--) {
        buffer_.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    return {};
}

void ByteStream::end_input() { end = true; }

bool ByteStream::input_ended() const { return end; }

size_t ByteStream::buffer_size() const { return buffer_.size(); }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return write_count_; }

size_t ByteStream::bytes_read() const { return read_count_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - buffer_size(); }

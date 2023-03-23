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
    size_t len = data.length();
    if (len > capacity_ - buffer_.size()) {
        len = capacity_ - buffer_.size();
    }
    write_count_ += len;
    buffer_.append(BufferList(std::move(string().assign(data.begin(),data.begin()+len))));
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = len;
    if (length > buffer_.size()) {
        length = buffer_.size();
    }
    string s=buffer_.concatenate();

    return string().assign(s.begin(), s.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
        size_t length = len;
    if (length > buffer_.size()) {
        length = buffer_.size();
    }
    read_count_ += length;
    buffer_.remove_prefix(length);
    return;

}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string temp = peek_output(len);
    pop_output(len);
    return temp;

}

void ByteStream::end_input() { end = true; }

bool ByteStream::input_ended() const { return end; }

size_t ByteStream::buffer_size() const { return buffer_.size(); }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return write_count_; }

size_t ByteStream::bytes_read() const { return read_count_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - buffer_size(); }

#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): _capacity(capacity) { /*DUMMY_CODE(capacity);*/ }

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    // return {};
    size_t n = data.size();
    if (n > _capacity - _buffer.size()) {
        n = _capacity - _buffer.size();
    }
    _bytes_written += n; 
    for (size_t i = 0; i < n; ++i) {
        _buffer.push_back(data[i]);
    } 
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    // return {};
    const size_t n = _buffer.size() >= len ? len : _buffer.size();
    return string().assign(_buffer.begin(), _buffer.begin() + n);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    size_t n = _buffer.size() >= len ? len : _buffer.size();
    _bytes_read += n;
    while (n--) {
        _buffer.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    // return {};
    const string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() {_input_ended = true;}

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }

#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader header = seg.header();
    if (header.syn && _syn) {
        return;
    }
    if (header.syn) {
        _syn = true;
        _isn = header.seqno.raw_value();
    }
    if (header.fin) {
        _fin = true;
    }
    string data = seg.payload().copy();
    uint64_t checkpoint = _reassembler.stream_out().bytes_written();
    uint64_t absolute_seqno = unwrap(header.seqno, WrappingInt32(_isn), checkpoint);
    uint64_t stream_index = header.syn ? 0 : absolute_seqno - 1;
    bool eof = header.fin;
    _reassembler.push_substring(data, stream_index, eof);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_syn) {
        return {};
    }
    uint64_t next_bytes = _reassembler.stream_out().bytes_written() + 1;
    uint64_t ackno = _reassembler.stream_out().input_ended() ? next_bytes + 1 : next_bytes;
    return wrap(ackno, WrappingInt32(_isn));
}

size_t TCPReceiver::window_size() const { 
    return  _reassembler.stream_out().bytes_read() + _capacity - _reassembler.stream_out().bytes_written();
}

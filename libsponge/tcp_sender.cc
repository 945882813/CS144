#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _RTO(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if (_syn_sent == false) {
        TCPSegment syn_seg;
        syn_seg.header().syn = true;
        send_segment(syn_seg);
        _syn_sent = true;
        return;
    }
    if (!_segments_outstanding.empty() && _segments_outstanding.front().header().syn) {
        return;
    }
    if (_stream.buffer_empty() && !_stream.eof()) {
        return;
    }
    if (_fin_sent) {
        return;
    }
    if (_receiver_windows_size) {
        while (_receiver_free_space) {
            TCPSegment seg;
            size_t payload_size = min({_stream.buffer_size(), 
                                        static_cast<size_t>(_receiver_free_space), 
                                        static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE)});
            seg.payload() = _stream.read(payload_size); //amazing
            if (_stream.eof() && static_cast<size_t>(_receiver_free_space) > payload_size) {
                seg.header().fin = true;
                _fin_sent = true;
            }
            send_segment(seg);
            if (_stream.buffer_empty()) {
                break;
            }
        }
    } else if (_receiver_free_space == 0) {
        // The zero-window-detect-segment should only be sent once (retransmition excute by tick function).
        // Before it is sent, _receiver_free_space is zero. Then it will be -1.
        TCPSegment seg;
        if (_stream.eof()) {
            seg.header().fin = true;
            _fin_sent = true;
        } else if (_stream.buffer_size()) {
            seg.payload() = _stream.read(1);
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t absolute_ackno = unwrap(ackno, _isn, _next_seqno);
    if (!ack_valid(absolute_ackno)) {
        return;
    }
    _receiver_windows_size = window_size;
    _receiver_free_space = window_size;
    while (!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= absolute_ackno) {
            _segments_outstanding.pop();
            _bytes_in_flight -= seg.length_in_sequence_space();
            // Do not do the following operations outside while loop.
            // Because if the ack is not corresponding to any segment in the segment_outstanding,
            // we should not restart the timer.
            _time_elapsed = 0;
            _RTO = _initial_retransmission_timeout;
            _consecutive_retransmission = 0;
        } else {
            break;
        }
    }
    if (!_segments_outstanding.empty()) {
        _receiver_free_space = static_cast<uint16_t>(absolute_ackno + static_cast<uint16_t>(window_size) -
                                                        unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno) - _bytes_in_flight);
    }
    if (_bytes_in_flight == 0) {
        _timer_runing = false;
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (!_timer_runing) {
        return;
    }
    _time_elapsed += ms_since_last_tick;
    if (_time_elapsed >= _RTO && !_segments_outstanding.empty()) {
        _segments_out.push(_segments_outstanding.front());
        _consecutive_retransmission++;
        if (_receiver_windows_size) {
            _RTO <<= 1;
        }
        _time_elapsed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();
    if (_syn_sent) {
        _receiver_free_space -= seg.length_in_sequence_space();
    }
    _segments_outstanding.push(seg);
    _segments_out.push(seg);
    if (!_timer_runing) {
        _timer_runing = true;
        _time_elapsed = 0;
    }
}

bool TCPSender::ack_valid(uint64_t absolute_ackno) {
    return absolute_ackno <= _next_seqno &&
            absolute_ackno >= unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno);
}

#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _first_unacceptable(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    _first_unread = _output.bytes_read();
    _first_unacceptable = _first_unread + _capacity;
    seg new_seg = {index, data.length(), data};
    _add_new_seg(new_seg, eof);
    _stitch_output();
    if (empty() && _eof) {
        _output.end_input();
    }
}

void StreamReassembler::_add_new_seg(seg &new_seg, const bool eof) {
    // check capacity limit, if unmeet limit, return
    // cut the bytes in NEW_SEG that will overflow the _CAPACITY
    // note that the EOF should also be cut
    // cut the bytes in NEW_SEG that are already in _OUTPUT
    // _HANDLE_OVERLAP()
    // update _EOF
    if (new_seg.index >= _first_unacceptable) {
        return;
    }
    bool eof_of_this_seg = eof;
    int overflow_bytes = new_seg.index + new_seg.length - _first_unacceptable;
    if (overflow_bytes > 0) {
        int new_length = new_seg.length - overflow_bytes;
        if (new_length <= 0) {
            return;
        }
        eof_of_this_seg = false;
        new_seg.length = new_length;
        new_seg.data = new_seg.data.substr(0, new_seg.length);
    }
    if (new_seg.index < _first_unassembled) {
        int new_length = new_seg.length - (_first_unassembled - new_seg.index);
        if (new_length <= 0) {
            return;
        }
        new_seg.length = new_length;
        new_seg.data = new_seg.data.substr(_first_unassembled - new_seg.index, new_seg.length);
        new_seg.index = _first_unassembled;
    }
    _handle_overlap(new_seg);
    _eof = _eof || eof_of_this_seg;
}

void StreamReassembler::_handle_overlap(seg &new_seg) {
    for (auto it = _stored_segs.begin(); it != _stored_segs.end();) {
        auto next_it = ++it;
        --it;
        if ((new_seg.index >= it->index && new_seg.index < it->index + it->length) ||
            (it->index >= new_seg.index && it->index < new_seg.index + new_seg.length)) {
                _merge_seg(new_seg, *it);
                _stored_segs.erase(it);
        }
        it = next_it;
    }
    _stored_segs.insert(new_seg);
}

void StreamReassembler::_merge_seg(seg &new_seg, const seg &other) {
    size_t new_begin = new_seg.index;
    size_t new_end = new_seg.index + new_seg.length;
    size_t other_begin = other.index;
    size_t other_end = other.index + other.length;
    string new_data;
    if (new_begin <= other_begin && new_end <= other_end) {
        new_data = new_seg.data + other.data.substr(new_end - other_begin, new_end - other_end);
    } else if (new_begin <= other_begin && new_end >= other_end) {
        new_data = new_seg.data;
    } else if (new_begin >= other_begin && new_end <= other_end) {
        new_data = other.data.substr(0, new_begin - other_begin) + new_seg.data + other.data.substr(new_end - other_begin, other_end - new_end);
    } else {
        new_data = other.data.substr(0, new_begin - other_begin) + new_seg.data;
    }
    new_seg.index = new_begin < other_begin ? new_begin : other_begin;
    new_seg.length = (new_end < other_end ? other_end : new_end) - new_seg.index;
    new_seg.data = new_data;
}

void StreamReassembler::_stitch_output() {
    // _FIRST_UNASSEMBLED is the expected next index_FIRST_UNASSEMBLED
    // compare _STORED_SEGS.begin()->index with
    // if equals, then _STITCH_ONE_SEG() and erase this seg from set
    // continue compare until not equal or empty
    while (!_stored_segs.empty() && _stored_segs.begin()->index == _first_unassembled) {
        _stitch_one_seg(*_stored_segs.begin());
        _stored_segs.erase(_stored_segs.begin());
    }
}

void StreamReassembler::_stitch_one_seg(const seg &new_seg) {
    _output.write(new_seg.data);
    _first_unassembled += new_seg.length;
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t unassembled_bytes = 0;
    for (auto it = _stored_segs.begin(); it != _stored_segs.end(); ++it) {
        unassembled_bytes += it->length;
    }
    return unassembled_bytes;
}

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }

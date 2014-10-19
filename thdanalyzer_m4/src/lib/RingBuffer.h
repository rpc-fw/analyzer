/*
 * Copyright (C) 2011 by Lauri Koponen, lauri.koponen@iki.fi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, UNICORN OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

namespace dsp
{

template<typename T, int LEN> class RingBufferStaticBuffer {
public:
	T* Buffer() { return _buffer; }
	const T* Buffer() const { return _buffer; }
private:
	T _buffer[LEN];
};

template<typename T> class RingBufferReferencedBuffer {
public:
	T* Buffer() { return _buffer; }
	const T* Buffer() const { return _buffer; }
private:
	T* _buffer;
};

template<typename T, int ADDRESS> class RingBufferMemoryBuffer {
public:
	T* Buffer() { return reinterpret_cast<T*>(ADDRESS); }
	const T* Buffer() const { return reinterpret_cast<T*>(ADDRESS); }
};

template<typename Parent, typename T, int LEN> class RingBufferImpl : public Parent {
public:

	class RingRange {
	public:

		RingRange()
		{
		}

		RingRange(T const* buffer, int const start, int const * const end, int const delay)
		  : _zero(0)
		{
			_buffer = const_cast<T*>(buffer);
			_pos = start;
			_end = end;
			_delay = delay;
		}

		bool isempty() const {
			return (_pos == *_end);
		}

		T& value() {
			if (_delay)
				return _zero;

			return _buffer[_pos];
		}

		T const& value() const {
			if (_delay)
				return _zero;

			return _buffer[_pos];
		}

		RingRange& advance() {
			if(_delay > 0) {
				_delay--;
				return *this;
			}

			if(isempty())
				return *this;

			++_pos;
			if(_pos == LEN)
				_pos = 0;
			
			return *this;
		}

		RingRange& advance(int numsamples) {
			int u = length();
			if(numsamples > u) numsamples = u;

			_pos += numsamples;
			if(_pos >= LEN)
				_pos -= LEN;

			return *this;
		}

		int length() const {
			if(_pos > *_end) {
				return *_end + (LEN - _pos);
			} else {
				return *_end - _pos;
			}
		}

		T* _buffer;
		T _zero;
		int _pos;
		int const * _end;
		int _delay;
	};

	class ReverseRingRange {
	public:
		// note that end is now the first item and start is the last item to be served
		ReverseRingRange(T const* buffer, int start, int end, int delay)
		  : _zero(0) {
			_buffer = const_cast<T*>(buffer);
			_pos = end;
			_end = start - 1;
			if (_end < 0)
				_end += LEN;
			_delay = delay;
		}
		bool isempty() const {
			return (_pos == _end);
		}
		T& value() {
			if (_delay)
				return _zero;

			return _buffer[_pos];
		}
		T const& value() const {
			if (_delay)
				return _zero;

			return _buffer[_pos];
		}
		ReverseRingRange& advance() {
			if(_delay > 0) {
				_delay--;
				return *this;
			}

			if(isempty())
				return *this;

			_pos--;
			if(_pos < 0)
				_pos += LEN;
			
			return *this;
		}

		ReverseRingRange& advance(int numsamples) {
			int u = length();
			if(numsamples > u) numsamples = u;

			_pos -= numsamples;
			if(_pos < 0)
				_pos += LEN;

			return *this;
		}

		int length() const {
			if(_pos < _end) {
				return (LEN - _end) + _pos;
			} else {
				return _pos - _end;
			}
		}

		T* _buffer;
		T _zero;
		int _pos;
		int _end;
		int _delay;
	};

	RingBufferImpl() {
		reset();
	}

	RingRange range() const {
		return RingRange(this->Buffer(), _start, &_end, 0);
	}

	ReverseRingRange revrange() const {
		int last = _end - 1;
		if (last < 0) {
			last += LEN;
		}
		return ReverseRingRange(this->Buffer(), _start, last, 0);
	}

	RingRange delayrange(int delay) const {
		int u = used();
		if(delay > u) {
			return RingRange(this->Buffer(), _start, _end, delay - u);
		}

		int curstart = _end - delay;
		if(curstart < 0)
			curstart += LEN;
		return RingRange(this->Buffer(), curstart, _end, 0);
	}

	RingRange delayrange(int delay, int length) const {
		int u = used();
		
		if (length > u)
		{
			length = u;
		}

		if(delay > u) {
			int totlen = length - (delay - u);
			if (totlen < 0)
			{
				totlen = 0;
			}
			int end = _start + totlen;
			if (end >= LEN)
			{
				end -= LEN;
			}
			return RingRange(this->Buffer(), _start, end, delay - u);
		}

		int curstart = _end - delay;
		if(curstart < 0)
			curstart += LEN;
		int end = curstart + length;
		if (end >= LEN)
		{
			end -= LEN;
		}
		return RingRange(this->Buffer(), curstart, end, 0);
	}

	void reset() {
		_start = _end = 0;
	}

	int used() const {
		if(_start > _end) {
			return _end + (LEN - _start);
		} else {
			return _end - _start;
		}
	}

	int free() const {
		return LEN - used();
	}

	void insert(T value) {
		if ((_end + 1 != _start) && (_end + 1 - LEN != _start)) {
			int prevend = _end;
			int end = prevend + 1;
			if (end == LEN) {
				end = 0;
			}
			this->Buffer()[prevend] = value;
			_end = end;
		}
	}

	// insert zeros
	void insert(int numsamples) {
		int f = free();
		if(f < numsamples) numsamples = f;
		if(numsamples < 1) return;

		if(_end + numsamples >= LEN) {
			memset(&this->Buffer()[_end], 0, (LEN - _end) * sizeof(T));
			numsamples -= LEN - _end;

			if ((_start > _end) || (_start == 0))
				_start = 1;

			_end = 0;
		}

		memset(&this->Buffer()[_end], 0, numsamples * sizeof(T));

		if (_start > _end && _start <= _end + numsamples)
			_start = _end + numsamples + 1;
		
		_end += numsamples;
	}

	// insert constant value
	void insert(T value, int numsamples) {
		int f = free();
		if(f < numsamples) numsamples = f;
		if(numsamples < 1) return;

		if(_end + numsamples >= LEN) {
			numsamples -= LEN - _end;

			while (_end < LEN) {
				this->Buffer()[_end++] = value;
			}

			if ((_start > _end) || (_start == 0))
				_start = 1;

			_end = 0;
		}

		int n = numsamples;

		if (_start > _end && _start <= _end + numsamples)
			_start = _end + numsamples + 1;

		while (n--) {
			this->Buffer()[_end++] = value;
		}
	}

	void insert(T *in, int numsamples, int stride = 1) {
		int f = free();
		if(f < numsamples) numsamples = f;
		if(numsamples < 1) return;

		if(stride == 1) {
			if(_end + numsamples >= LEN) {
				memcpy(&this->Buffer()[_end], in, (LEN - _end) * sizeof(T));
				numsamples -= LEN - _end;
				in += LEN - _end;

				if ((_start > _end) || (_start == 0))
					_start = 1;

				_end = 0;
			}

			memcpy(&this->Buffer()[_end], in, numsamples * sizeof(T));

			if (_start > _end && _start <= _end + numsamples)
				_start = _end + numsamples + 1;
			
			_end += numsamples;

		} else {
			if(_end + numsamples > LEN) {
				for(int i = 0; i < LEN - _end; ++i)
					this->Buffer()[_end + i] = in[i * stride];
				numsamples -= LEN - _end;
				in += (LEN - _end) * stride;

				if (_start > _end)
					_start = 1;

				_end = 0;
			}

			for(int i = 0; i < numsamples; ++i)
				this->Buffer()[_end + i] = in[i * stride];

			if (_start > _end && _start <= _end + numsamples)
				_start = _end + numsamples + 1;
			
			_end += numsamples;
		}
	}

	// reserve space at end, leave uninitialized
	T* reserve(int numsamples) {
		int f = free();
		if(f < numsamples) numsamples = f;
		if(numsamples < 1) return 0;

		T* ptr = &this->Buffer()[_end];

		if(_end + numsamples >= LEN) {
			numsamples -= LEN - _end;

			if ((_start > _end) || (_start == 0))
				_start = 1;

			_end = 0;
		}

		if (_start > _end && _start <= _end + numsamples)
			_start = _end + numsamples + 1;

		_end += numsamples;

		return ptr;
	}

	bool has(int numsamples) const {
		if(used() >= numsamples)
			return true;
		return false;
	}

	const T* oldestPtr() const {
		return &this->Buffer()[_start];
	}

	T oldest() const {
		return *oldestPtr();
	}

	const T* latestPtr() const {
		int i = _end - 1;
		if (i < 0) i += LEN;
		return &this->Buffer()[i];
	}

	T latest() const {
		return *latestPtr();
	}

	void get(T *out, int numsamples) {
		int u = used();
		if(numsamples > u) {
			memset(out + u, 0, (numsamples - u) * sizeof(T));
			numsamples = u;
		}

		if(numsamples >= LEN - _start) {
			memcpy(out, &this->Buffer()[_start], (LEN - _start) * sizeof(T));
			out += LEN - _start;
			numsamples -= LEN - _start;
			_start = 0;
		}

		memcpy(out, &this->Buffer()[_start], numsamples * sizeof(T));
		_start += numsamples;
	}
	
	void copy(T *out, int numsamples) const {
		int curstart = _start;
		int u = used();
		if(numsamples > u) {
			memset(out + u, 0, (numsamples - u) * sizeof(T));
			numsamples = u;
		}

		if(numsamples >= LEN - curstart) {
			memcpy(out, &this->Buffer()[curstart], (LEN - curstart) * sizeof(T));
			out += LEN - curstart;
			numsamples -= LEN - curstart;
			curstart = 0;
		}

		if(numsamples)
			memcpy(out, &this->Buffer()[curstart], numsamples * sizeof(T));
	}

	int copy(T *out, int numsamples, int delay) const {
		int u = used();
		if(numsamples + delay > u) {
			return -1;
		}

		int curstart = _end - delay - numsamples;
		if(curstart < 0)
			curstart += LEN;

		if(numsamples >= LEN - curstart) {
			memcpy(out, &this->Buffer()[curstart], (LEN - curstart) * sizeof(T));
			out += LEN - curstart;
			numsamples -= LEN - curstart;
			curstart = 0;
		}

		if(numsamples)
			memcpy(out, &this->Buffer()[curstart], numsamples * sizeof(T));

		return 0;
	}

	void advance(int numsamples) {
		int u = used();
		if(numsamples > u) numsamples = u;

		_start += numsamples;
		if(_start >= LEN)
			_start -= LEN;
	}

protected:
	int _start, _end;
};

template <typename T, int LEN>
class RingBufferStatic : public RingBufferImpl<RingBufferStaticBuffer<T, LEN>, T, LEN>
{
};

template <typename T, int LEN>
class RingBufferReferenced : public RingBufferImpl<RingBufferReferencedBuffer<T>, T, LEN>
{
public:
	RingBufferReferenced(T* buffer)
	{
		this->_buffer = buffer;
	}
};

template <typename T, int LEN, int ADDRESS>
class RingBufferMemory : public RingBufferImpl<RingBufferMemoryBuffer<T, ADDRESS>, T, LEN>
{
};

}; // namespace dsp

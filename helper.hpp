/*
 * helper.hpp
 *
 * Miscellaneous helpful little tidbits...
 */

/*
 * Copyright (c) 2015, James Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __HELPER_HPP__
#define __HELPER_HPP__

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <iomanip>

#include <exception>

#include <string>
#include <vector>
#include <map>

// *************************************************************************

namespace Helper {

struct string_repr_out {
	string_repr_out( const std::string &str ) : s(str) {}
	void write_on( std::ostream & ) const;

	std::string s;
};

inline std::ostream &operator <<( std::ostream & o, const string_repr_out & sro ) {
	sro.write_on( o );
	return o;
}

template <class T>
inline const T &repr( const T &v ) { return v; }

inline string_repr_out repr( const char *s ) { return string_repr_out(s); }
inline string_repr_out repr( const std::string & s ) { return string_repr_out(s); }

// *************************************************************************

class NotFoundException : public std::exception {
public:

	const char *what() throw ();
};

template <class KEY_T, class VAL_T>
inline const VAL_T &find_or_throw( const std::map<KEY_T,VAL_T> &m, const KEY_T &target ) {
	auto i = m.find( target );
	if( i == m.end() ) {
		throw NotFoundException();
	}
	return i->second;
};

template <class KEY_T, class VAL_T>
inline VAL_T &find_or_throw( std::map<KEY_T,VAL_T> &m, const KEY_T &target ) {
	auto i = m.find( target );
	if( i == m.end() ) {
		throw NotFoundException();
	}
	return i->second;
};


// *************************************************************************

template <class T>
class Coord {
public:
	Coord() : x(), y() {}
	Coord( T _x, T _y ) : x(_x), y(_y) {}
	T x;
	T y;

};

template <class T>
std::ostream &operator<<( std::ostream &o, const Coord<T> &c ) {
	o << "{ " << c.x << " x " << c.y << " }";
	return o;
};


template <class T>
class Bounds {
public:
	typedef Coord<T> CT;
	Bounds( const CT &_tl, const CT &_br) : tl(_tl), br(_br) {}
	Bounds( T x1, T y1, T x2, T y2 ) : tl(x1,y1), br(x2,y2) {}

	bool contains( const CT &pos ) const {
		return tl.x <= pos.x && tl.y <= pos.y &&  pos.x <= br.x && pos.y <= br.y;
	}

	void expand( const CT &pos ) {
		if( pos.x < tl.x ) tl.x = pos.x;
		if( pos.y < tl.y ) tl.y = pos.y;
		if( pos.x > br.x ) br.x = pos.x;
		if( pos.y > br.y ) br.y = pos.y;
	}
	CT tl;
	CT br;
};

template <class T>
std::ostream &operator<<( std::ostream &o, const Bounds<T> &b ) {
	o << "{ " << b.tl.x << " x " << b.tl.y << " / " << b.br.x << " x " << b.br.y << " }";
	return o;
};

// *************************************************************************

typedef const char * CCP;
inline const char *advance_ws(CCP &source, std::string &dest) {
	const char *space = source ? strchr(source, ' ') : 0;
	if (space) {
		dest = std::string(source, space - source);
		source = space + 1;
	} else {
		dest = source;
		source = 0;
	}
	return source;
};

// *************************************************************************

template <class MAP_T>
struct _map_keys_out {
	_map_keys_out( const MAP_T&c, const std::string &s ) : container(c), sep(s) {}
	const MAP_T&container;
	std::string sep;
};


template <class STREAM_T, class MAP_T>
STREAM_T &operator <<( STREAM_T &o, const _map_keys_out<MAP_T> &_mko ) {
	bool first = true;
	for( auto i = _mko.container.begin(); i != _mko.container.end(); i++ ) {
		if( first ) {
			first = false;
			o << i->first;
		} else {
			o << _mko.sep << i->first;
		}
	}
};

template <class MAP_T>
_map_keys_out<MAP_T> map_keys_out( const MAP_T &c, const std::string &sep = " " ) {
	return _map_keys_out<MAP_T>( c, sep );
};

// *************************************************************************

}; // namespace Helper


// *************************************************************************


#endif // __HELPER_HPP__

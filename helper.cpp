/*
 *  helper.cpp
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

#include "helper.hpp"

// *************************************************************************

namespace Helper {

void string_repr_out::write_on( std::ostream &o ) const {

	o << "\"";
	const char *cp = s.c_str();
	const char *end = cp + s.size();

	while( cp < end ) {

		switch( *cp ) {
				case '\n':	o << "\\n"; break;
				case '\r':	o << "\\r"; break;
				case '\0':	o << "\\0"; break;
				case '\t':	o << "\\t"; break;
				case '\\':
				case '\'':
				case '\"':
					o << "\\" << *cp;
					break;
				default: {
					char c = *cp;
					if( c < 32 ) {
						char hi = '0' + (c & 0x0f);
						char lo = '0' + ((c >> 4) & 0x0f);
						o << "\\x" << hi << lo;
					} else {
						o << c;
					}
				}
		}
		cp++;
	}

	o << "\"";
};


}; // namespace Helper


// *************************************************************************



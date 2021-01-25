/*
 	 pixels are mapped rather strangely for G13 buffer...

	  byte 0 contains column 0 / row 0 - 7
	  byte 1 contains column 1 / row 0 - 7

	 so the masks for each pixel are laid out as below (ByteOffset.PixelMask)

	 00.01 01.01 02.01 ...
	 00.02 01.02 02.02 ...
	 00.04 01.04 02.04 ...
	 00.08 01.08 02.08 ...
	 00.10 01.10 02.10 ...
	 00.20 01.20 02.20 ...
	 00.40 01.40 02.40 ...
	 00.80 01.80 02.80 ...
	 A0.01 A1.01 A2.01 ...
 */

#include "g13.h"
#include "logo.h"

using namespace std;

namespace G13 {

void G13_Device::init_lcd() {
  int error = libusb_control_transfer(handle, 0, 9, 1, 0, 0, 0, 1000);
  if(error) {
    G13_LOG( error,  "Error when initializing lcd endpoint" );
  }
}

void G13_Device::write_lcd( unsigned char *data, size_t size ) {
  init_lcd();
  if(size != G13_LCD_BUFFER_SIZE) {
	G13_LOG( error, "Invalid LCD data size " << size << ", should be " << G13_LCD_BUFFER_SIZE );
    return;
  }
  unsigned char buffer[G13_LCD_BUFFER_SIZE + 32];
  memset(buffer, 0, G13_LCD_BUFFER_SIZE + 32);
  buffer[0] = 0x03;
  memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);
  int bytes_written;
  int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT, buffer, G13_LCD_BUFFER_SIZE + 32, &bytes_written, 1000);
  if(error)
	  G13_LOG( error, "Error when transferring image: " << error << ", " << bytes_written << " bytes written" );
}

void G13_Device::write_lcd_file( const string &filename ) {
  filebuf *pbuf;
  ifstream filestr;
  size_t size;

  filestr.open(filename.c_str());
  pbuf = filestr.rdbuf();

  size = pbuf->pubseekoff(0, ios::end, ios::in);
  pbuf->pubseekpos(0, ios::in);

  char buffer[size];

  pbuf->sgetn(buffer, size);

  filestr.close();
  write_lcd( (unsigned char *)buffer, size );
}

void G13_LCD::image(unsigned char *data, int size) {
	_keypad.write_lcd( data, size );
}

G13_LCD::G13_LCD( G13_Device &keypad ) : _keypad(keypad) {
    cursor_col = 0;
    cursor_row = 0;
    text_mode = 0;
}

void G13_LCD::image_setpixel(unsigned row, unsigned col) {
	unsigned offset = image_byte_offset(row, col); // col + (row /8 ) * BYTES_PER_ROW * 8;
	unsigned char mask = 1 << ((row) & 7);

	if (offset >= G13_LCD_BUF_SIZE) {
		G13_LOG( error, "bad offset " << offset << " for " << (row) << " x " << (col) );
		return;
	}

	image_buf[offset] |= mask;
}

void G13_LCD::image_clearpixel(unsigned row, unsigned col) {

	unsigned offset = image_byte_offset(row, col); // col + (row /8 ) * BYTES_PER_ROW * 8;
	unsigned char mask = 1 << ((row) & 7);

	if (offset >= G13_LCD_BUF_SIZE) {
		G13_LOG( error, "bad offset " << offset << " for " << (row) << " x " << (col) );
		return;
	}
	image_buf[offset] &= ~mask;
}


void G13_LCD::write_pos(int row, int col ) {
	cursor_row = row;
	cursor_col = col;
	if( cursor_col >= G13_LCD_COLUMNS ) {
		cursor_col = 0;
	}
	if( cursor_row >= G13_LCD_TEXT_ROWS ) {
		cursor_row = 0;
	}
}
void G13_LCD::write_char( char c, int row, int col ) {
	if( row == -1 ) {
		row = cursor_row;
		col = cursor_col;
		cursor_col += _keypad.current_font().width();
		if( cursor_col >= G13_LCD_COLUMNS ) {
			cursor_col = 0;
			if( ++cursor_row >= G13_LCD_TEXT_ROWS ) {
				cursor_row = 0;
			}
		}
	}

	unsigned offset = image_byte_offset( row*G13_LCD_TEXT_CHEIGHT, col ); //*_keypad._current_font->_width );
	if( text_mode ) {
		memcpy( & image_buf[offset], &_keypad.current_font().char_data(c).bits_inverted, _keypad.current_font().width() );
	} else {
		memcpy( & image_buf[offset], &_keypad.current_font().char_data(c).bits_regular, _keypad.current_font().width() );
	}
}

void G13_LCD::write_string( const char *str ) {
	G13_LOG( info, "writing \"" << str << "\"" );
	while( *str ) {
		if( *str == '\n' ) {
			cursor_col = 0;
			if( ++cursor_row >= G13_LCD_TEXT_ROWS ) {
				cursor_row = 0;
			}
		} else if( *str == '\t' ) {
			cursor_col += 4 - (cursor_col % 4) ;
			if( ++cursor_col >= G13_LCD_COLUMNS ) {
				cursor_col = 0;
				if( ++cursor_row >= G13_LCD_TEXT_ROWS ) {
					cursor_row = 0;
				}
			}
		} else {
			write_char(*str);
		}
		++str;
	}
	image_send();
}

void G13_LCD::image_test( int x, int y ) {

	int row = 0, col = 0;
	if( y >= 0 ) {
		image_setpixel( x, y );
	} else {
		image_clear();
		switch( x ) {
		case 1:
			for( row = 0; row < G13_LCD_ROWS; ++row ) {
				col = row;
				image_setpixel( row, col );
				image_setpixel( row, G13_LCD_COLUMNS-col );
			}
			break;

		case 2:
		default:
			for( row = 0; row < G13_LCD_ROWS; ++row ) {
				col = row;
				image_setpixel( row, 8 );
				image_setpixel( row, G13_LCD_COLUMNS - 8 );
				image_setpixel( row, G13_LCD_COLUMNS / 2 );
				image_setpixel( row, col );
				image_setpixel( row, G13_LCD_COLUMNS-col );
			}
			break;

		}
	}
	image_send();
}


} // namespace G13


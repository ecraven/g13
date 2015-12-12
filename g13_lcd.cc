
#include "g13.h"
#include "logo.h"
using namespace std;

namespace G13 {

void G13_KeyPad::init_lcd() {
  int error = libusb_control_transfer(handle, 0, 9, 1, 0, G13_NULL, 0, 1000);
  if(error) {
    cerr << "Error when initialising lcd endpoint" << endl;
  }
}

void G13_KeyPad::write_lcd(libusb_context *ctx, unsigned char *data, size_t size) {
  init_lcd();
  if(size != G13_LCD_BUFFER_SIZE) {
    cerr << "Invalid LCD data size " << size << ", should be " << G13_LCD_BUFFER_SIZE;
    return;
  }
  unsigned char buffer[G13_LCD_BUFFER_SIZE + 32];
  memset(buffer, 0, G13_LCD_BUFFER_SIZE + 32);
  buffer[0] = 0x03;
  memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);
  int bytes_written;
  int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT, buffer, G13_LCD_BUFFER_SIZE + 32, &bytes_written, 1000);
  if(error)
    cerr << "Error when transfering image: " << error << ", " << bytes_written << " bytes written" << endl;
}

void G13_KeyPad::write_lcd_file(libusb_context *ctx, const string &filename) {
	G13_KeyPad *g13 = this;
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
  write_lcd(ctx, (unsigned char *)buffer, size);
}

void G13_LCD::image(unsigned char *data, int size) {
	_keypad.write_lcd( _keypad._manager.ctx, data, size);
}

G13_LCD::G13_LCD( G13_KeyPad &keypad ) : _keypad(keypad) {
    cursor_col = 0;
    cursor_row = 0;
    text_mode = 0;
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
		cursor_col += _keypad.current_font->_width;
		if( cursor_col >= G13_LCD_COLUMNS ) {
			cursor_col = 0;
			if( ++cursor_row >= G13_LCD_TEXT_ROWS ) {
				cursor_row = 0;
			}
		}
	}

	unsigned offset = image_byte_offset( row*G13_LCD_TEXT_CHEIGHT, col ); //*_keypad.current_font->_width );
	if( text_mode ) {
		memcpy( & image_buf[offset], &_keypad.current_font->chars[c].bits_inverted, _keypad.current_font->_width );
	} else {
		memcpy( & image_buf[offset], &_keypad.current_font->chars[c].bits_regular, _keypad.current_font->_width );
	}
}

void G13_LCD::write_string( const char *str ) {
	std::cout << "writing \"" << str << "\"" << std::endl;
	while( *str ) {
		if( *str == '\n' ) {
			cursor_col = 0;
			if( ++cursor_row >= G13_LCD_TEXT_ROWS ) {
				cursor_row = 0;
			}
		} else if( *str == '\t' ) {
			cursor_col += 4 - (cursor_col % 4) ;
			if( ++cursor_col >= G13_LCD_TEXT_COLUMNS ) {
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


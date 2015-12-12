#ifndef __G13_H__
#define __G13_H__

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/shared_ptr.hpp>
#include <iomanip>
#include <sys/stat.h>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <linux/uinput.h>
#include <fcntl.h>
#include <string>
#include <map>

#define G13_NULL 0

namespace G13 {

#define G13_INTERFACE 0
#define G13_KEY_ENDPOINT 1
#define G13_LCD_ENDPOINT 2
#define G13_KEY_READ_TIMEOUT 0
#define G13_VENDOR_ID 0x046d
#define G13_PRODUCT_ID 0xc21c
#define G13_REPORT_SIZE 8
#define G13_LCD_BUFFER_SIZE 0x3c0
#define G13_NUM_KEYS 40

enum stick_mode_t { STICK_ABSOLUTE, STICK_RELATIVE, STICK_KEYS, STICK_CALCENTER, STICK_CALBOUNDS, STICK_CALNORTH };
enum stick_key_t { STICK_LEFT, STICK_UP, STICK_DOWN, STICK_RIGHT };

const size_t G13_LCD_COLUMNS = 160;
const size_t G13_LCD_ROWS = 48;
const size_t G13_LCD_BYTES_PER_ROW = G13_LCD_COLUMNS/8;
const size_t G13_LCD_BUF_SIZE = G13_LCD_ROWS * G13_LCD_BYTES_PER_ROW;

//const size_t TEXT_CWIDTH = 5;
const size_t G13_LCD_TEXT_CWIDTH = 8;
const size_t G13_LCD_TEXT_CHEIGHT = 8;
const size_t G13_LCD_TEXT_COLUMNS = 160 / G13_LCD_TEXT_CWIDTH;
const size_t G13_LCD_TEXT_ROWS = 160 / G13_LCD_TEXT_CHEIGHT;

class G13_Profile;
class G13_KeyPad;
class G13_Manager;

class G13_Action {
public:
	G13_Action( G13_KeyPad & keypad ) : _keypad(keypad) {}
	virtual ~G13_Action();

	virtual void act( G13_KeyPad &, bool is_down ) = 0;

	void act( bool is_down ) { act( keypad(), is_down ); }

	G13_KeyPad & keypad() { return _keypad; }
	const G13_KeyPad & keypad() const { return _keypad; }

private:
	G13_KeyPad & _keypad;
};

class G13_Action_Keys : public G13_Action {
public:
	G13_Action_Keys( G13_KeyPad & keypad, const std::string &keys );
	virtual ~G13_Action_Keys();

	virtual void act( G13_KeyPad &, bool is_down );

	std::vector<int> _keys;
};

class G13_Action_PipeOut : public G13_Action {
public:
	G13_Action_PipeOut( G13_KeyPad & keypad, const std::string &out );
	virtual ~G13_Action_PipeOut();

	virtual void act( G13_KeyPad &, bool is_down );

	std::string _out;
};

class G13_Action_Command : public G13_Action {
public:
	G13_Action_Command( G13_KeyPad & keypad, const std::string &cmd );
	virtual ~G13_Action_Command();

	virtual void act( G13_KeyPad &, bool is_down );

	std::string _cmd;
};


typedef boost::shared_ptr<G13_Action> G13_ActionPtr;

class G13_Key {
public:

	const std::string &name() const { return _name; }

	G13_Profile & 	mode() const { return _mode; }
	int 			index() const { return _index.index; }
	int 			mapped_key() const { return _mapped_key; }

	void 			set_mapping( int key );


	void parse_key( unsigned char *byte, G13_KeyPad *g13);

	G13_ActionPtr _action;

private:
	struct KeyIndex {
		KeyIndex( int key ) :
			index(key),
			offset( key / 8 ),
			mask( 1 << (key % 8) )
		{}

		int index;
		unsigned char offset;
		unsigned char mask;
	};
	friend class G13_Profile;
	G13_Key( G13_Profile & mode, const std::string &name, int index ) :
		_mode(mode),
		_name(name),
		_index(index),
		_mapped_key(KEY_A),
		_should_parse(true)
	{
	}

	G13_Key( G13_Profile & mode, const G13_Key &key ) :
		_mode(mode),
		_name(key._name),
		_index(key._index),
		_mapped_key(key._mapped_key),
		_should_parse(key._should_parse)
	{}


	G13_Profile & _mode;
	std::string _name;
	KeyIndex _index;
	int _mapped_key;
	bool _should_parse;
};

/*!
 * Represents a set of configured key mappings
 *
 * This allows a keypad to have multiple configured
 * profiles and switch between them easily
 */
class G13_Profile {
public:
	G13_Profile(G13_KeyPad &keypad) : _keypad(keypad) {
		_init_keys();
	}

	int mapping( int key ) const {
		if( key >= 0 && key < _keys.size() ) {
			return _keys[key].mapped_key();
		}
		return -1;
	}
	G13_Key * find_key( const std::string &keyname );

	void parse_keys( unsigned char *buf );

	G13_KeyPad &_keypad;
	std::vector<G13_Key> _keys;

protected:
	void _init_keys();
};

typedef boost::shared_ptr<G13_Profile> ProfilePtr;

class G13_FontChar {
public:
	static const int CHAR_BUF_SIZE = 8;
	enum FONT_FLAGS { FF_ROTATE= 0x01 };

	G13_FontChar() {
		memset(bits_regular, 0, CHAR_BUF_SIZE);
		memset(bits_inverted, 0, CHAR_BUF_SIZE);
	}
	void set_character( unsigned char *data, int width, unsigned flags );
	unsigned char bits_regular[CHAR_BUF_SIZE];
	unsigned char bits_inverted[CHAR_BUF_SIZE];
};

class G13_Font {
public:
	G13_Font();
	G13_Font( const std::string &name, unsigned int width = 8 );

	std::string _name;
	unsigned int _width;

	void set_character( unsigned int c, unsigned char *data );

	template < class ARRAY_T, class FLAGST >
	void install_font( ARRAY_T &data, FLAGST flags, int first = 0 );

	G13_FontChar chars[256];

	//unsigned char font_basic[256][8];
	//unsigned char font_inverted[256][8];
};
typedef boost::shared_ptr<G13_Font> FontPtr;

class G13_LCD {
public:

	// pixels are mapped rather strangely for G13 buffer...
	//
	//  byte 0 contains column 0 / row 0 - 7
	//  byte 1 contains column 1 / row 0 - 7
	//
	// so the masks for each pixel are laid out as below (ByteOffset.PixelMask)
	//
	// 00.01 01.01 02.01 ...
	// 00.02 01.02 02.02 ...
	// 00.04 01.04 02.04 ...
	// 00.08 01.08 02.08 ...
	// 00.10 01.10 02.10 ...
	// 00.20 01.20 02.20 ...
	// 00.40 01.40 02.40 ...
	// 00.80 01.80 02.80 ...
	// A0.01 A1.01 A2.01 ...
	G13_LCD( G13_KeyPad &keypad );

	G13_KeyPad &_keypad;
	unsigned char image_buf[G13_LCD_BUF_SIZE+8];
	unsigned cursor_row;
	unsigned cursor_col;
	int text_mode;

  void image(unsigned char *data, int size);
  void image_send() {
	  image( image_buf, G13_LCD_BUF_SIZE );
  }

  void image_test( int x, int y );
  void image_clear() {
	  memset( image_buf, 0, G13_LCD_BUF_SIZE );
  }

  unsigned image_byte_offset( unsigned row, unsigned col ) {
  	  return col + (row /8 ) * G13_LCD_BYTES_PER_ROW * 8;
    }

  void image_setpixel( unsigned row, unsigned col ) {
	unsigned offset = image_byte_offset( row, col ); // col + (row /8 ) * BYTES_PER_ROW * 8;
	unsigned char mask = 1 << ((row)&7);

	if( offset >= G13_LCD_BUF_SIZE ) {
		std::cerr << "bad offset " << offset << " for "
				<< (row) << " x " << (col) << std::endl;
		return;
	}

	image_buf[ offset ] |= mask;
  }


  void image_clearpixel( unsigned row, unsigned col ) {

	unsigned offset = image_byte_offset( row, col ); // col + (row /8 ) * BYTES_PER_ROW * 8;
	unsigned char mask = 1 << ((row)&7);

	if( offset >= G13_LCD_BUF_SIZE ) {
		std::cerr << "bad offset " << offset << " for "
				<< (row) << " x " << (col) << std::endl;
		return;
	}
	image_buf[ offset ] &= ~mask;
  }


  void write_char( char c, int row = -1, int col = -1);
  void write_string( const char *str );
  void write_pos(int row, int col );

};

class G13_CommandException : public std::exception {
public:
	G13_CommandException( const std::string &reason ) : _reason(reason) {}
	virtual ~G13_CommandException() throw () {}
	virtual const char *what() const throw () { return _reason.c_str(); }

	std::string _reason;
};

class G13_Stick;
template <class T>
class Coord {
public:
	Coord() : x(), y() {}
	Coord( T _x, T _y ) : x(_x), y(_y) {}
	T x;
	T y;

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

typedef Coord<int> G13_StickCoord;
typedef Bounds<int> G13_StickBounds;
typedef Coord<double> G13_ZoneCoord;
typedef Bounds<double> G13_ZoneBounds;

class G13_StickZone {
public:

	const std::string &name() const { return _name; }

	//G13_Profile & 	mode() const { return _mode; }

	void 			set_mapping( int key );

	void parse_key( unsigned char *byte, G13_KeyPad *g13);

	void test( const G13_ZoneCoord &loc );
	bool _active;

	G13_ActionPtr _action;
	std::string _name;

	G13_ZoneBounds _bounds;

	G13_StickZone( const std::string &name,  const G13_ZoneBounds &, G13_ActionPtr = 0 );

	bool operator == ( const G13_StickZone &other ) const {
		return _name == other._name;
	}

};

typedef boost::shared_ptr< G13_StickZone> G13_StickZonePtr;


class G13_Stick {
public:
	G13_Stick( G13_KeyPad &keypad );

	void parse_joystick(unsigned char *buf);

	void set_mode( stick_mode_t );
	G13_StickZone *zone( const std::string &, bool create=false );
	void _recalc_calibrated();
	void remove_zone( const G13_StickZone &zone );

	G13_KeyPad &_keypad;
	std::vector<G13_StickZone> _zones;

	G13_StickBounds _bounds;
	G13_StickCoord _center_pos;
	G13_StickCoord _north_pos;

	G13_StickCoord _current_pos;

	stick_mode_t _stick_mode;

};

struct G13_KeyPad {

  G13_KeyPad(G13_Manager &manager, libusb_device_handle *handle, int id);


  void init_lcd();
  void write_lcd(libusb_context *ctx, unsigned char *data, size_t size);

  void write_lcd_file(libusb_context *ctx, const std::string &filename);

  FontPtr switch_to_font( const std::string &name );
  void switch_to_profile( const std::string &name );
  ProfilePtr profile( const std::string &name );

  // int mapping(int key) { return current_profile->mapping(key); }

  void command(char const *str);
  void read_commands();
  int read_keys();
  void parse_joystick(unsigned char *buf);

  G13_ActionPtr make_action( const std::string & );

  G13_LCD &lcd() { return _lcd; }



  const char *fifo_name() { return _fifo_name.c_str(); }

  void _init_fonts();


  void set_key_color( int red, int green, int blue );
  void set_mode_leds( int leds );


  void register_context(libusb_context *ctx);

  void cleanup();

  libusb_device_handle *handle;
  int uinput_file;
  int id;
  int fifo;
  int fifo_out;


  std::string _fifo_name;

  std::map<std::string,FontPtr> _fonts;
  FontPtr current_font;
  std::map<std::string,ProfilePtr> _profiles;
  ProfilePtr current_profile;



  // int map[G13_NUM_KEYS];
  G13_Manager &_manager;
  G13_LCD _lcd;
  G13_Stick _stick;


  bool keys[G13_NUM_KEYS];
  bool is_set(int key) { return keys[key]; }
  bool update(int key, bool v) {
    bool old = keys[key];
    keys[key] = v;
    return old != v;
  }
};

/*!
 * top level class, holds what would otherwise be in global variables
 */

class G13_Manager {
public:
	G13_Manager();

	void init_keynames();
	void display_keys();
	void discover_g13s(libusb_device **devs, ssize_t count, std::vector<G13_KeyPad*>& g13s);
	void cleanup();

	int run();
	void set_logo( const std::string &fn ) { logo_filename = fn; }

	std::string logo_filename;
	libusb_device **devs;

	libusb_context *ctx;
	std::vector<G13_KeyPad*> g13s;

	std::map<int,std::string> key_to_name;
	std::map<std::string,int> name_to_key;
	std::map<int,std::string> input_key_to_name;
	std::map<std::string,int> input_name_to_key;
};


} // namespace G13

#endif // __G13_H__

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

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

enum stick_mode_t { STICK_ABSOLUTE, /*STICK_RELATIVE,*/ STICK_KEYS };
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
class g13_keypad;
class G13_Manager;

class G13_Key {
public:

	const std::string &name() const { return _name; }

	G13_Profile & 	mode() const { return _mode; }
	int 			index() const { return _index.index; }
	int 			mapped_key() const { return _mapped_key; }

	void 			set_mapping( int key ) { _mapped_key = key; }

	void parse_key( unsigned char *byte, g13_keypad *g13);

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
	G13_Profile(g13_keypad &keypad) : _keypad(keypad) {
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

	g13_keypad &_keypad;
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
	G13_LCD( g13_keypad &keypad );

	g13_keypad &_keypad;
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


struct g13_keypad {
  libusb_device_handle *handle;
  int uinput_file;
  int id;
  int fifo;
  stick_mode_t stick_mode;
  int stick_keys[4];

  std::string _fifo_name;

  std::map<std::string,FontPtr> _fonts;
  FontPtr current_font;
  FontPtr switch_to_font( const std::string &name );

  std::map<std::string,ProfilePtr> _profiles;
  ProfilePtr current_profile;

  g13_keypad(G13_Manager &manager, libusb_device_handle *handle, int id);
  // int map[G13_NUM_KEYS];
  G13_Manager &_manager;
  G13_LCD _lcd;

  G13_LCD &lcd() { return _lcd; }


  void switch_to_profile( const std::string &name );
  ProfilePtr profile( const std::string &name );

  int mapping(int key) {
	return current_profile->mapping(key);
  }

  bool keys[G13_NUM_KEYS];
  bool is_set(int key) {
    return keys[key];
  }
  bool update(int key, bool v) {
    bool old = keys[key];
    keys[key] = v;
    return old != v;
  }

  void command(char const *str);

  const char *fifo_name() {
    return _fifo_name.c_str();
  }

  void _init_fonts();
};

class G13_Manager {
public:
	G13_Manager();

	void init_keynames();
	void display_keys();
	void discover_g13s(libusb_device **devs, ssize_t count, std::vector<g13_keypad*>& g13s);
	void cleanup(int n = 0);

	int run();
	void set_logo( const std::string &fn ) { logo_filename = fn; }

	std::string logo_filename;
	libusb_device **devs;

	libusb_context *ctx;
	std::vector<g13_keypad*> g13s;

	std::map<int,std::string> key_to_name;
	std::map<std::string,int> name_to_key;
	std::map<int,std::string> input_key_to_name;
	std::map<std::string,int> input_name_to_key;
};

enum G13_KEYS {
    /* byte 3 */
    G13_KEY_G1 = 0,
    G13_KEY_G2,
    G13_KEY_G3,
    G13_KEY_G4,

    G13_KEY_G5,
    G13_KEY_G6,
    G13_KEY_G7,
    G13_KEY_G8,

    /* byte 4 */
    G13_KEY_G9,
    G13_KEY_G10,
    G13_KEY_G11,
    G13_KEY_G12,

    G13_KEY_G13,
    G13_KEY_G14,
    G13_KEY_G15,
    G13_KEY_G16,

    /* byte 5 */
    G13_KEY_G17,
    G13_KEY_G18,
    G13_KEY_G19,
    G13_KEY_G20,

    G13_KEY_G21,
    G13_KEY_G22,
    G13_KEY_UNDEF1,
    G13_KEY_LIGHT_STATE,

    /* byte 6 */
    G13_KEY_BD,
    G13_KEY_L1,
    G13_KEY_L2,
    G13_KEY_L3,

    G13_KEY_L4,
    G13_KEY_M1,
    G13_KEY_M2,
    G13_KEY_M3,

    /* byte 7 */
    G13_KEY_MR,
    G13_KEY_LEFT,
    G13_KEY_DOWN,
    G13_KEY_TOP,

    G13_KEY_UNDEF3,
    G13_KEY_LIGHT,
    G13_KEY_LIGHT2,
    G13_KEY_MISC_TOGGLE
};

void register_g13(libusb_context *ctx, g13_keypad *dev);

} // namespace G13


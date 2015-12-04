#include <string>
#include <map>

#define null 0

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

struct g13_keypad {
  libusb_device_handle *handle;
  int uinput_file;
  int id;
  int fifo;
  stick_mode_t stick_mode;
  int stick_keys[4];

  std::string _fifo_name;
  unsigned cursor_row;
  unsigned cursor_col;
  int text_mode;
  unsigned char font_basic[128][8];
  unsigned char font_inverted[128][8];

  unsigned char image_buf[G13_LCD_BUF_SIZE+8];

  g13_keypad(libusb_device_handle *handle, int id);
  int map[G13_NUM_KEYS];

  void set_mapping(int mapping[G13_NUM_KEYS]) {
    for(int i = 0; i < G13_NUM_KEYS; i++)
      map[i] = mapping[i];
  }
  int mapping(int key) {
    return map[key];
  }
  bool keys[G13_NUM_KEYS];
  bool is_set(int key) {
    return keys[key];
  }
  bool update(int key, bool v) {
    bool old = keys[key];
    keys[key] = v;
    //    cout << key << ": " << old << ", " << v << endl;
    return old != v;
  }
  void command(char const *str);
  void image(unsigned char *data, int size);

  const char *fifo_name() {
    return _fifo_name.c_str();
  }

  void image_test( int x, int y );
  void image_clear() {
	  memset( image_buf, 0, G13_LCD_BUF_SIZE );
  }
  void image_send() {
	  image( image_buf, G13_LCD_BUF_SIZE );
  }

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
  void _init_fonts();
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


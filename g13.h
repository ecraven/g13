#include <string>
#include <map>
#include "g13map.h"

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
struct g13_keypad {
  libusb_device_handle *handle;
  int uinput_file;
  int id;
  int fifo;
  stick_mode_t stick_mode;
  int stick_keys[4];
  g13_keypad(libusb_device_handle *handle, int id);

  g13map* keymap;

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
#define CONTROL_DIR std::string("/tmp/")
  const char *fifo_name() {
    return (CONTROL_DIR + "g13-" + boost::lexical_cast<std::string>(id)).c_str();
  }
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


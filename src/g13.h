#ifndef __G13_H__
#define __G13_H__


#include "helper.hpp"

#include <boost/log/trivial.hpp>

#include <libusb-1.0/libusb.h>

#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <linux/uinput.h>
#include <fcntl.h>

// *************************************************************************

namespace G13 {

#define G13_LOG( level, message ) BOOST_LOG_TRIVIAL( level ) << message
#define G13_OUT( message ) BOOST_LOG_TRIVIAL( info ) << message

const size_t G13_INTERFACE = 0;
const size_t G13_KEY_ENDPOINT = 1;
const size_t G13_LCD_ENDPOINT = 2;
const size_t G13_KEY_READ_TIMEOUT = 0;
const size_t G13_VENDOR_ID = 0x046d;
const size_t G13_PRODUCT_ID = 0xc21c;
const size_t G13_REPORT_SIZE = 8;
const size_t G13_LCD_BUFFER_SIZE = 0x3c0;
const size_t G13_NUM_KEYS = 40;


const size_t G13_LCD_COLUMNS = 160;
const size_t G13_LCD_ROWS = 48;
const size_t G13_LCD_BYTES_PER_ROW = G13_LCD_COLUMNS/8;
const size_t G13_LCD_BUF_SIZE = G13_LCD_ROWS * G13_LCD_BYTES_PER_ROW;
const size_t G13_LCD_TEXT_CHEIGHT = 8;
const size_t G13_LCD_TEXT_ROWS = 160 / G13_LCD_TEXT_CHEIGHT;

enum stick_mode_t { STICK_ABSOLUTE, STICK_RELATIVE, STICK_KEYS, STICK_CALCENTER, STICK_CALBOUNDS, STICK_CALNORTH };

typedef int LINUX_KEY_VALUE;
const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;

typedef int G13_KEY_INDEX;

// *************************************************************************

using Helper::repr;
using Helper::find_or_throw;

// *************************************************************************

class G13_Action;
class G13_Stick;
class G13_LCD;
class G13_Profile;
class G13_Device;
class G13_Manager;

class G13_CommandException : public std::exception {
public:
	G13_CommandException( const std::string &reason ) : _reason(reason) {}
	virtual ~G13_CommandException() throw () {}
	virtual const char *what() const throw () { return _reason.c_str(); }

	std::string _reason;
};

// *************************************************************************

/*! holds potential actions which can be bound to G13 activity
 *
 */
class G13_Action {
public:
	G13_Action( G13_Device & keypad ) : _keypad(keypad) {}
	virtual ~G13_Action();

	virtual void act( G13_Device &, bool is_down ) = 0;
	virtual void dump( std::ostream & ) const = 0;

	void act( bool is_down ) { act( keypad(), is_down ); }

	G13_Device & keypad() { return _keypad; }
	const G13_Device & keypad() const { return _keypad; }

	G13_Manager &manager();
	const G13_Manager &manager() const;

private:
	G13_Device & _keypad;
};


/*!
 * action to send one or more keystrokes
 */
class G13_Action_Keys : public G13_Action {
public:
	G13_Action_Keys( G13_Device & keypad, const std::string &keys );
	virtual ~G13_Action_Keys();

	virtual void act( G13_Device &, bool is_down );
	virtual void dump( std::ostream & ) const;

	std::vector<LINUX_KEY_VALUE> _keys;
};

/*!
 * action to send a string to the output pipe
 */
class G13_Action_PipeOut : public G13_Action {
public:
	G13_Action_PipeOut( G13_Device & keypad, const std::string &out );
	virtual ~G13_Action_PipeOut();

	virtual void act( G13_Device &, bool is_down );
	virtual void dump( std::ostream & ) const;

	std::string _out;
};

/*!
 * action to send a command to the g13
 */
class G13_Action_Command : public G13_Action {
public:
	G13_Action_Command( G13_Device & keypad, const std::string &cmd );
	virtual ~G13_Action_Command();

	virtual void act( G13_Device &, bool is_down );
	virtual void dump( std::ostream & ) const;

	std::string _cmd;
};


typedef boost::shared_ptr<G13_Action> G13_ActionPtr;

// *************************************************************************
template <class PARENT_T>
class G13_Actionable {
public:
	G13_Actionable( PARENT_T &parent_arg, const std::string &name ) :
		_parent_ptr(&parent_arg), _name(name)
	{}
	virtual ~G13_Actionable() { _parent_ptr = 0; }

	G13_ActionPtr 			action() const { return _action; }
	const std::string &		name() const { return _name; }
	PARENT_T &				parent() { return *_parent_ptr; }
	const PARENT_T &		parent() const { return *_parent_ptr; }
	G13_Manager &			manager() { return _parent_ptr->manager(); }
	const G13_Manager &		manager() const { return _parent_ptr->manager(); }

	virtual void			set_action( const G13_ActionPtr &action ) {
		_action = action;
	}

protected:

	std::string _name;
	G13_ActionPtr _action;

private:
	PARENT_T *_parent_ptr;
};

// *************************************************************************
/*! manages the bindings for a G13 key
 *
 */
class G13_Key : public G13_Actionable<G13_Profile> {
public:


	void					dump( std::ostream &o ) const;
	G13_KEY_INDEX			index() const { return _index.index; }

	void 					parse_key( unsigned char *byte, G13_Device *g13 );

protected:


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

	// G13_Profile is the only class able to instantiate G13_Keys
	friend class G13_Profile;

	G13_Key( G13_Profile & mode, const std::string &name, int index ) : G13_Actionable<G13_Profile>( mode, name ),
		_index(index),
		_should_parse(true)
	{
	}

	G13_Key( G13_Profile & mode, const G13_Key &key ) :  G13_Actionable<G13_Profile>( mode, key.name() ),
		_index(key._index),
		_should_parse(key._should_parse)
	{
		set_action(key.action());
	}


	KeyIndex _index;
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
	G13_Profile(G13_Device &keypad, const std::string &name_arg ) : _keypad(keypad), _name(name_arg) {
		_init_keys();
	}
	G13_Profile(const G13_Profile &other, const std::string &name_arg ) : _keypad(other._keypad), _name(name_arg), _keys(other._keys)
	{
	}


	// search key by G13 keyname
	G13_Key * 			find_key( const std::string &keyname );

	void				dump( std::ostream &o ) const;

	void 				parse_keys( unsigned char *buf );
	const std::string &name() const { return _name; }

	const G13_Manager &manager() const;

protected:
	G13_Device &_keypad;
	std::vector<G13_Key> _keys;
	std::string _name;

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


	void set_character( unsigned int c, unsigned char *data );

	template < class ARRAY_T, class FLAGST >
	void install_font( ARRAY_T &data, FLAGST flags, int first = 0 );

	const std::string &name() const { return _name; }
	unsigned int width() const { return _width; }

	const G13_FontChar &char_data( unsigned int x ) { return _chars[x]; }
protected:
	std::string _name;
	unsigned int _width;

	G13_FontChar _chars[256];

	//unsigned char font_basic[256][8];
	//unsigned char font_inverted[256][8];
};
typedef boost::shared_ptr<G13_Font> FontPtr;

class G13_LCD {
public:


	G13_LCD( G13_Device &keypad );

	G13_Device &_keypad;
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

  void image_setpixel( unsigned row, unsigned col );
	  void image_clearpixel( unsigned row, unsigned col );



  void write_char( char c, int row = -1, int col = -1);
  void write_string( const char *str );
  void write_pos(int row, int col );

};
using Helper::repr;

typedef Helper::Coord<int> G13_StickCoord;
typedef Helper::Bounds<int> G13_StickBounds;
typedef Helper::Coord<double> G13_ZoneCoord;
typedef Helper::Bounds<double> G13_ZoneBounds;

// *************************************************************************

class G13_StickZone :  public G13_Actionable<G13_Stick> {
public:

	G13_StickZone( G13_Stick &, const std::string &name,  const G13_ZoneBounds &, G13_ActionPtr = 0 );

	bool operator == ( const G13_StickZone &other ) const { return _name == other._name; }

	void				dump( std::ostream & ) const;

	void parse_key( unsigned char *byte, G13_Device *g13);
	void test( const G13_ZoneCoord &loc );
	void set_bounds( const G13_ZoneBounds &bounds ) { _bounds = bounds; }


protected:

	bool _active;

	G13_ZoneBounds _bounds;

};

typedef boost::shared_ptr< G13_StickZone> G13_StickZonePtr;

// *************************************************************************

class G13_Stick {
public:
	G13_Stick( G13_Device &keypad );

	void 				parse_joystick(unsigned char *buf);

	void 				set_mode( stick_mode_t );
	G13_StickZone *		zone( const std::string &, bool create=false );
	void 				remove_zone( const G13_StickZone &zone );

	const std::vector<G13_StickZone> & zones() const { return _zones; }

	void				dump( std::ostream & ) const;

protected:

	void _recalc_calibrated();

	G13_Device &_keypad;
	std::vector<G13_StickZone> _zones;

	G13_StickBounds _bounds;
	G13_StickCoord _center_pos;
	G13_StickCoord _north_pos;

	G13_StickCoord _current_pos;

	stick_mode_t _stick_mode;

};

// *************************************************************************

class G13_Device {
public:

  G13_Device( G13_Manager &manager, libusb_device_handle *handle, int id );


  G13_Manager &manager() { return _manager; }
  const G13_Manager &manager() const { return _manager; }

  G13_LCD &lcd() { return _lcd; }
  const G13_LCD &lcd() const { return _lcd; }
  G13_Stick &stick() { return _stick; }
  const G13_Stick &stick() const { return _stick; }

  FontPtr switch_to_font( const std::string &name );
  void switch_to_profile( const std::string &name );
  ProfilePtr profile( const std::string &name );

  void dump(std::ostream &, int detail = 0 );
  void command(char const *str);

  void read_commands();
  void read_config_file( const std::string &filename );

  int read_keys();
  void parse_joystick(unsigned char *buf);

  G13_ActionPtr make_action( const std::string & );

  void set_key_color( int red, int green, int blue );
  void set_mode_leds( int leds );


  void send_event( int type, int code, int val );
  void write_output_pipe( const std::string &out );

  void write_lcd( unsigned char *data, size_t size );

  bool is_set(int key) ;
  bool update(int key, bool v) ;

  // used by G13_Manager
  void cleanup();
  void register_context(libusb_context *ctx);
  void write_lcd_file( const std::string &filename);

  G13_Font &current_font() { return *_current_font; }
  G13_Profile &current_profile() { return *_current_profile; }

  int id_within_manager() const { return _id_within_manager; }

  typedef boost::function<void ( const char * )> COMMAND_FUNCTION;
  typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

protected:

  void _init_fonts();
  void init_lcd();
  void _init_commands();


  //typedef void (COMMAND_FUNCTION)( G13_Device*, const char *, const char * );
  CommandFunctionTable _command_table;

  struct timeval _event_time;
  struct input_event _event;

  int _id_within_manager;
  libusb_device_handle *handle;
  libusb_context *ctx;

  int _uinput_fid;

  int _input_pipe_fid;
  std::string _input_pipe_name;
  int _output_pipe_fid;
  std::string _output_pipe_name;

  std::map<std::string,FontPtr> _fonts;
  FontPtr _current_font;
  std::map<std::string,ProfilePtr> _profiles;
  ProfilePtr _current_profile;

  G13_Manager &_manager;
  G13_LCD _lcd;
  G13_Stick _stick;


  bool keys[G13_NUM_KEYS];
};

// *************************************************************************


/*!
 * top level class, holds what would otherwise be in global variables
 */

class G13_Manager {
public:
	G13_Manager();

	G13_KEY_INDEX find_g13_key_value( const std::string &keyname ) const;
	std::string find_g13_key_name( G13_KEY_INDEX ) const;

	LINUX_KEY_VALUE find_input_key_value( const std::string &keyname ) const;
	std::string find_input_key_name( LINUX_KEY_VALUE ) const;

	void set_logo( const std::string &fn ) { logo_filename = fn; }
	int run();

	std::string string_config_value( const std::string &name ) const;
	void set_string_config_value( const std::string &name, const std::string &val );

	std::string make_pipe_name( G13_Device *d, bool is_input );

	void set_log_level( ::boost::log::trivial::severity_level lvl );
	void set_log_level( const std::string & );

protected:

	void init_keynames();
	void display_keys();
	void discover_g13s(libusb_device **devs, ssize_t count, std::vector<G13_Device*>& g13s);
	void cleanup();



	std::string logo_filename;
	libusb_device **devs;

	libusb_context *ctx;
	std::vector<G13_Device*> g13s;


	std::map<G13_KEY_INDEX,std::string> g13_key_to_name;
	std::map<std::string,G13_KEY_INDEX> g13_name_to_key;
	std::map<LINUX_KEY_VALUE,std::string> input_key_to_name;
	std::map<std::string,LINUX_KEY_VALUE> input_name_to_key;

	std::map<std::string, std::string> _string_config_values;

	static bool running;
	static void set_stop(int);
};

// *************************************************************************

// inlines

inline G13_Manager &G13_Action::manager() {
	return _keypad.manager();
}

inline const G13_Manager &G13_Action::manager() const{
	return _keypad.manager();
}

inline bool G13_Device::is_set(int key)
{
	return keys[key];
}

inline bool G13_Device::update(int key, bool v) {
    bool old = keys[key];
    keys[key] = v;
    return old != v;
  }

inline const G13_Manager &G13_Profile::manager() const
{
	return _keypad.manager();
}

// *************************************************************************


} // namespace G13

#endif // __G13_H__

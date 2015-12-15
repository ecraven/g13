/* This file contains code for managing keys and profiles
 *
 */
#include "g13.h"


using namespace std;

namespace G13 {


// *************************************************************************

void G13_Device::parse_joystick(unsigned char *buf ) {
	_stick.parse_joystick(buf);
}

G13_Stick::G13_Stick( G13_Device &keypad ) :
		_keypad(keypad),
		_bounds(0,0,255,255),
		_center_pos(127,127),
		_north_pos( 127, 0 )
{
    _stick_mode = STICK_KEYS;

    auto add_zone = [this, &keypad]( const std::string &name, double x1, double y1, double x2, double y2 ) {
    	_zones.push_back( G13_StickZone( *this, "STICK_"+name,
    										G13_ZoneBounds( x1, y1, x2, y2 ),
											G13_ActionPtr(
													new G13_Action_Keys( keypad, "KEY_" + name ) )
													)
    			);
    };

    add_zone( "UP", 0.0, 0.1, 1.0, 0.3 );
    add_zone( "DOWN", 0.0, 0.7, 1.0, 0.9 );
    add_zone( "LEFT", 0.0, 0.0, 0.2, 1.0 );
    add_zone( "RIGHT", 0.8, 0.0, 1.0, 1.0 );
    add_zone( "PAGEUP", 0.0, 0.0, 1.0, 0.1 );
    add_zone( "PAGEDOWN", 0.0, 0.9, 1.0, 1.0 );

}

G13_StickZone *G13_Stick::zone( const std::string &name, bool create ) {

	BOOST_FOREACH( G13_StickZone &zone, _zones ) {
		if( zone.name() == name ) {
			return &zone;
		}
	}
	if( create ) {
		_zones.push_back( G13_StickZone( *this, name, G13_ZoneBounds( 0.0, 0.0, 0.0, 0.0 ) ) );
		return zone(name);
	}
	return 0;
}

void G13_Stick::set_mode( stick_mode_t  m ) {
	if( m == _stick_mode )
		return;
	if( _stick_mode == STICK_CALCENTER || _stick_mode == STICK_CALBOUNDS || _stick_mode == STICK_CALNORTH ) {
		_recalc_calibrated();
	}
	_stick_mode = m;
	switch( _stick_mode ) {
	case STICK_CALBOUNDS:
		_bounds.tl = G13_StickCoord( 255, 255 );
		_bounds.br = G13_StickCoord( 0, 0 );
		break;
	}
}

void G13_Stick::_recalc_calibrated() {
}

void G13_Stick::remove_zone( const G13_StickZone &zone ) {
	G13_StickZone target(zone);
	_zones.erase(std::remove(_zones.begin(), _zones.end(), target), _zones.end());

}
void G13_Stick::dump( std::ostream &out ) const {
	BOOST_FOREACH( const G13_StickZone &zone, _zones ) {
		zone.dump( out );
		out << endl;
	}
}

void G13_StickZone::dump( std::ostream & out ) const {
	out << "   " << setw(20) << name() << "   " << _bounds << "  ";
	if( action() ) {
		action()->dump( out );
	} else {
		out << " (no action)";
	}
}

void G13_StickZone::test( const G13_ZoneCoord &loc ) {
	if( !_action ) return;
	bool prior_active = _active;
	_active = _bounds.contains( loc );
	if( !_active ) {
		if( prior_active ) {
			// cout << "exit stick zone " << _name << std::endl;
			_action->act( false );
		}
	} else {
		// cout << "in stick zone " << _name << std::endl;
		_action->act( true );
	}
}

G13_StickZone::G13_StickZone( G13_Stick &stick, const std::string &name,  const G13_ZoneBounds &b, G13_ActionPtr action) :
	G13_Actionable<G13_Stick>( stick, name ), _bounds(b), _active(false)
{
	set_action( action );

}

void G13_Stick::parse_joystick(unsigned char *buf) {

	_current_pos.x = buf[1];
	_current_pos.y = buf[2];

	// update targets if we're in calibration mode
	switch (_stick_mode) {

	case STICK_CALCENTER:
		_center_pos = _current_pos;
		return;

	case STICK_CALNORTH:
		_north_pos = _current_pos;
		return;

	case STICK_CALBOUNDS:
		_bounds.expand( _current_pos );
		return;
	};

	// determine our normalized position
	double dx = 0.5;
	if (_current_pos.x <= _center_pos.x) {
		dx = _current_pos.x - _bounds.tl.x;
		dx /= (_center_pos.x - _bounds.tl.x) * 2;
	} else {
		dx = _bounds.br.x - _current_pos.x;
		dx /= (_bounds.br.x - _center_pos.x ) * 2;
		dx = 1.0 - dx;
	}
	double dy = 0.5;
	if (_current_pos.y <= _center_pos.y) {
		dy = _current_pos.y - _bounds.tl.y;
		dy /= (_center_pos.y - _bounds.tl.y) * 2;
	} else {
		dy = _bounds.br.y - _current_pos.y;
		dy /= (_bounds.br.y -_center_pos.y ) * 2;
		dy = 1.0 - dy;
	}

	G13_LOG( trace, "x=" << _current_pos.x << " y=" << _current_pos.y << " dx=" << dx << " dy=" << dy );
	G13_ZoneCoord jpos(dx, dy);
	if (_stick_mode == STICK_ABSOLUTE) {
		_keypad.send_event( EV_ABS, ABS_X, _current_pos.x );
		_keypad.send_event( EV_ABS, ABS_Y, _current_pos.y );

	} else if (_stick_mode == STICK_KEYS) {

		BOOST_FOREACH( G13_StickZone &zone, _zones ) {
			zone.test(jpos);
		}
		return;

	} else {
		/*    send_event(g13->uinput_file, EV_REL, REL_X, stick_x/16 - 8);
		 send_event(g13->uinput_file, EV_REL, REL_Y, stick_y/16 - 8);*/
	}
}

} // namespace G13


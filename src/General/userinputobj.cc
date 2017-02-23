/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/


#include "userinputobj.h"
#include "uistring.h"

void UserInputObj::setValue( const char* s )
{ setText(s); }


void UserInputObj::addItem( const uiString& s )
{ setText(s.getFullString()); }


bool UserInputObj::notifyValueChanging( const CallBack& cb )
{ return notifyValueChanging_( cb ); }


bool UserInputObj::notifyValueChanged( const CallBack& cb )
{ return notifyValueChanged_( cb ); }


bool UserInputObj::notifyUpdateRequested( const CallBack& cb )
{ return notifyUpdateRequested_( cb ); }


bool UserInputObj::update( const DataInpSpec& s )
{ return update_(s); }

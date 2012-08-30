/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Feb 2004
-*/

static const char* rcsID mUnusedVar = "$Id: usrinputobj.cc,v 1.1 2012-08-30 10:57:16 cvskris Exp $";

#include "userinputobj.h"

void UserInputObj::setValue( const char* s )
{ setText(s); }


void UserInputObj::addItem( const char* s )
{ setText(s); }


bool UserInputObj::notifyValueChanging( const CallBack& cb )
{ return notifyValueChanging_( cb ); }


bool UserInputObj::notifyValueChanged( const CallBack& cb )
{ return notifyValueChanged_( cb ); }


bool UserInputObj::notifyUpdateRequested( const CallBack& cb )
{ return notifyUpdateRequested_( cb ); }


bool UserInputObj::update( const DataInpSpec& s )
{ return update_(s); }

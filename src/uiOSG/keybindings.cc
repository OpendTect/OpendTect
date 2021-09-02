/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "keybindings.h"
#include "settings.h"
#include "separstr.h"
#include "ascstream.h"
#include "strmprov.h"
#include "oddirs.h"
#include <iostream>

#include <osgGeo/TrackballManipulator>

FixedString KeyBindings::sSettingsKey() {return "dTect.MouseControls.Default";}

FixedString KeyBindings::sName()    { return "Name"; }
FixedString KeyBindings::sRotate()  { return "Rotate"; }
FixedString KeyBindings::sPan()	    { return "Pan"; }
FixedString KeyBindings::sZoom()    { return "Zoom"; }
FixedString KeyBindings::sControl() { return "Control"; }
FixedString KeyBindings::sShift()   { return "Shift"; }
FixedString KeyBindings::sLeft()    { return "Left"; }
FixedString KeyBindings::sRight()   { return "Right"; }
FixedString KeyBindings::sMiddle()  { return "Middle"; }
FixedString KeyBindings::sNone()    { return "None"; }


KeyBindMan::KeyBindMan()
    : curkeybinding_( "Default" )
{
    od_istream strm( mGetSetupFileName("MouseControls") );
    if ( !strm.isOK() ) return;
    ascistream astrm( strm );

    if ( atEndOfSection(astrm) ) astrm.next();
    if ( astrm.hasKeyword("Default") )
    {
	curkeybinding_ = astrm.value();
        astrm.next();
        astrm.next();
    }

    for ( int idx=0; !atEndOfSection(astrm); idx++ )
    {
        IOPar iopar( astrm );
        KeyBindings keybind;
        if ( iopar.hasKey(KeyBindings::sName()) )
        {
            BufferString res;
            iopar.get( KeyBindings::sName(), res );
	    keybind.name_ = res;
        }
        if ( iopar.hasKey(KeyBindings::sZoom()) )
	    iopar.get( KeyBindings::sZoom(), keybind.zoom_ );
        if ( iopar.hasKey(KeyBindings::sRotate()) )
	    iopar.get( KeyBindings::sRotate(), keybind.rotate_ );
        if ( iopar.hasKey(KeyBindings::sPan()) )
	    iopar.get( KeyBindings::sPan(), keybind.pan_ );
        while ( !atEndOfSection(astrm) ) astrm.next();
        astrm.next();
	keyset_ += new KeyBindings( keybind );
    }

    Settings::common().get( KeyBindings::sSettingsKey(), curkeybinding_ );
    setKeyBindings( curkeybinding_ );
}


KeyBindMan::~KeyBindMan()
{
    deepErase( keyset_ );
}


void KeyBindMan::setTrackballManipulator( osgGeo::TrackballManipulator* manip )
{
    manipulator_ = manip;
    updateTrackballKeyBindings();
}


void KeyBindMan::setKeyBindings( const char* name )
{
    curkeybinding_ = name;

    KeyBindings keys;
    for ( int idx=0; idx<keyset_.size(); idx++ )
    {
	if ( keyset_[idx] && keyset_[idx]->name_ == name )
        {
	    keys = *keyset_[idx];
            break;
        }
    }

    BufferString res;
    FileMultiString fms;
    res = keys.zoom_;
    fms = FileMultiString(res);
    zoom_.mousebut_ = fms.size() ? fms[0] : KeyBindings::sRight();
    zoom_.keybut_ = fms.size() > 1 ? fms[1] : KeyBindings::sNone();

    res = keys.rotate_;
    fms = FileMultiString(res);
    rotate_.mousebut_ = fms.size() ? fms[0] : KeyBindings::sLeft();
    rotate_.keybut_ = fms.size() > 1 ? fms[1] : KeyBindings::sNone();

    res = keys.pan_;
    fms = FileMultiString(res);
    pan_.mousebut_ = fms.size() ? fms[0] : KeyBindings::sMiddle();
    pan_.keybut_ = fms.size() > 1 ? fms[1] : KeyBindings::sNone();

    updateTrackballKeyBindings();
}


void KeyBindMan::getAllKeyBindings( BufferStringSet& keys )
{
    for ( int idx=0; idx<keyset_.size(); idx++ )
	keys.add( keyset_[idx]->name_ );
}


static osgGeo::TrackballManipulator::MouseButton
				getMouseButton( BufferString& mousebutstr )
{
    if ( mousebutstr == KeyBindings::sRight() )
	return osgGeo::TrackballManipulator::RightButton;
    if ( mousebutstr == KeyBindings::sMiddle() )
	return osgGeo::TrackballManipulator::MiddleButton;

    return osgGeo::TrackballManipulator::LeftButton;
}


static int getModKeyMask( BufferString& keybutstr )
{

    if ( keybutstr == KeyBindings::sShift() )
	return osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( keybutstr == KeyBindings::sControl() )
	return osgGA::GUIEventAdapter::MODKEY_CTRL;

    return 0;
}


void KeyBindMan::updateTrackballKeyBindings()
{
    if ( !manipulator_ )
	return;

    manipulator_->setRotateMouseButton( getMouseButton(rotate_.mousebut_) );
    manipulator_->setPanMouseButton( getMouseButton(pan_.mousebut_) );
    manipulator_->setZoomMouseButton( getMouseButton(zoom_.mousebut_) );
    manipulator_->setRotateModKeyMask( getModKeyMask(rotate_.keybut_) );
    manipulator_->setPanModKeyMask( getModKeyMask(pan_.keybut_) );
    manipulator_->setZoomModKeyMask( getModKeyMask(zoom_.keybut_) );
}

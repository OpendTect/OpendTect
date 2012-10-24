/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "keybindings.h"
#include "settings.h"
#include "separstr.h"
#include "ascstream.h"
#include "strmprov.h"
#include "oddirs.h"
#include <iostream>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>


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
    : dozoom(false)
    , shiftpress(false)
    , ctrlpress(false)
    , curkeyb( "Default" )
{
    StreamData sd = StreamProvider( mGetSetupFileName("MouseControls") )
			.makeIStream();
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm );
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( astrm.hasKeyword("Default") )
    {
        curkeyb = astrm.value();
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
            keybind.name = res;
        }
        if ( iopar.hasKey(KeyBindings::sZoom()) )
            iopar.get( KeyBindings::sZoom(), keybind.zoom );
        if ( iopar.hasKey(KeyBindings::sRotate()) )
            iopar.get( KeyBindings::sRotate(), keybind.rotate );
        if ( iopar.hasKey(KeyBindings::sPan()) )
            iopar.get( KeyBindings::sPan(), keybind.pan );
        while ( !atEndOfSection(astrm) ) astrm.next();
        astrm.next();
        keyset += new KeyBindings( keybind );
    }
    sd.close();

    mSettUse(get,"dTect.MouseControls","Default",curkeyb);
    setKeyBindings( curkeyb, false );
}


KeyBindMan::~KeyBindMan()
{
    deepErase( keyset );
}


void KeyBindMan::setKeyBindings( const char* name, bool saveinsett )
{
    curkeyb = name;
    if ( saveinsett )
    {
	mSettUse(set,"dTect.MouseControls","Default",curkeyb);
	Settings::common().write();
    }

    KeyBindings keys;
    for ( int idx=0; idx<keyset.size(); idx++ )
    {
        if ( keyset[idx] && keyset[idx]->name == name )
        {
            keys = *keyset[idx];
            break;
        }
    }

    BufferString res;
    FileMultiString fms;
    res = keys.zoom;
    fms = FileMultiString(res);
    zoom.mousebut = fms.size() ? fms[0] : KeyBindings::sRight();
    zoom.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone();

    res = keys.rotate;
    fms = FileMultiString(res);
    rotate.mousebut = fms.size() ? fms[0] : KeyBindings::sLeft();
    rotate.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone();

    res = keys.pan;
    fms = FileMultiString(res);
    pan.mousebut = fms.size() ? fms[0] : KeyBindings::sMiddle();
    pan.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone();
}


const SoEvent* KeyBindMan::processSoEvent( const SoEvent* const event, 
					   bool isviewing, bool popupenab )
{
    const SoType type( event->getTypeId() );
    const SoKeyboardEvent* keyevent = 0;
    if ( type.isDerivedFrom( SoKeyboardEvent::getClassTypeId() ) )
        keyevent = (SoKeyboardEvent*)event;

    if ( curkeyb == "Default" )
        return event;

    if ( keyevent )
    {
        switch ( keyevent->getKey() )
        {
            case SoKeyboardEvent::LEFT_SHIFT:
            case SoKeyboardEvent::RIGHT_SHIFT:
            {
                shiftpress = keyevent->getState() == SoButtonEvent::DOWN;
            } break;
            case SoKeyboardEvent::LEFT_CONTROL:
            case SoKeyboardEvent::RIGHT_CONTROL:
            {
                ctrlpress = keyevent->getState() == SoButtonEvent::DOWN;
            } break;
	    default:
	    	break;
        }
    }

    useownevent = false;
    SoMouseButtonEvent* mouseevent = 0;
    if ( type.isDerivedFrom( SoMouseButtonEvent::getClassTypeId() ) )
        mouseevent = (SoMouseButtonEvent*)event;

    if ( isviewing && mouseevent )
    {
        BufferString buttxt;
        switch ( mouseevent->getButton() )
        {
            case SoMouseButtonEvent::BUTTON1:
                buttxt = KeyBindings::sLeft();
                break;
            case SoMouseButtonEvent::BUTTON2:
                buttxt = KeyBindings::sRight();
                break;
            case SoMouseButtonEvent::BUTTON3:
                buttxt = KeyBindings::sMiddle();
                break;
            case SoMouseButtonEvent::BUTTON4:
                break;
            case SoMouseButtonEvent::BUTTON5:
                break;
            default:
                break;
        }

        if ( buttxt.size() )
        {
            if ( correctButtonsPushed( zoom, buttxt ) )
                doZoom( mouseevent );
            else if ( correctButtonsPushed( pan, buttxt ) )
                doPan( mouseevent );
            else if ( correctButtonsPushed( rotate, buttxt ) )
                doRotate( mouseevent );
        }

        if ( popupenab ) useownevent = true;
        return useownevent ? mouseevent :  0;
    }


    SoLocation2Event* locevent = 0;
    if ( type.isDerivedFrom(SoLocation2Event::getClassTypeId()) )
        locevent = (SoLocation2Event*)event;

    if ( locevent )
    {
        locevent->setCtrlDown( dozoom );
        locevent->setShiftDown( false );
        return locevent;
    }

    return event;
}


bool KeyBindMan::correctButtonsPushed( EventButton but, const char* txt )
{
    return ( but.mousebut == txt ) && (
                    ( but.keybut == "Shift" && shiftpress ) ||
                    ( but.keybut == "Control" && ctrlpress ) ||
                    ( but.keybut == "None" && !shiftpress && !ctrlpress ) );
}


void KeyBindMan::doRotate( SoMouseButtonEvent* event )
{
    event->setButton( SoMouseButtonEvent::BUTTON1 );
    event->setCtrlDown( FALSE );
    event->setShiftDown( FALSE );
    event->setAltDown( FALSE );
    dozoom = false;
    useownevent = true;
}


void KeyBindMan::doPan( SoMouseButtonEvent* event )
{
    event->setButton( SoMouseButtonEvent::BUTTON3 );
    event->setCtrlDown( FALSE );
    event->setShiftDown( FALSE );
    event->setAltDown( FALSE );
    dozoom = false;
    useownevent = true;
}


void KeyBindMan::doZoom( SoMouseButtonEvent* event )
{
    event->setButton( SoMouseButtonEvent::BUTTON3 );
    event->setCtrlDown( TRUE );
    event->setShiftDown( FALSE );
    event->setAltDown( FALSE );
    dozoom = true;
    useownevent = true;
}


void KeyBindMan::getAllKeyBindings( BufferStringSet& keys )
{
    for ( int idx=0; idx<keyset.size(); idx++ )
	keys.add( keyset[idx]->name );
}

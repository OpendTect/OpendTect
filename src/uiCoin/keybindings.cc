/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          July 2003
 RCS:           $Id: keybindings.cc,v 1.3 2003-11-07 12:22:01 bert Exp $
________________________________________________________________________

-*/

#include "keybindings.h"
#include "settings.h"
#include "separstr.h"
#include "ascstream.h"
#include <fstream>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>


const char* KeyBindings::sName =        "Name";
const char* KeyBindings::sRotate =      "Rotate";
const char* KeyBindings::sPan =         "Pan";
const char* KeyBindings::sZoom =        "Zoom";
const char* KeyBindings::sControl =     "Control";
const char* KeyBindings::sShift =       "Shift";
const char* KeyBindings::sLeft =        "Left";
const char* KeyBindings::sRight =       "Right";
const char* KeyBindings::sMiddle =      "Middle";
const char* KeyBindings::sNone =        "None";


KeyBindMan::KeyBindMan()
    : dozoom(false)
    , shiftpress(false)
    , ctrlpress(false)
    , curkeyb( "Default" )
{
    ifstream strm( GetDataFileName("MouseControls") );
    if ( !strm ) return;
    ascistream astrm( strm );
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( astrm.hasKeyword("Default") )
    {
        curkeyb = astrm.value();
        astrm.next();
        astrm.next();
    }

    for ( int idx=0; !atEndOfSection(astrm); idx++ )
    {
        IOPar iopar( astrm, false );
        KeyBindings keybind;
        if ( iopar.hasKey(KeyBindings::sName) )
        {
            BufferString res;
            iopar.get( KeyBindings::sName, res );
            keybind.name = res;
        }
        if ( iopar.hasKey(KeyBindings::sZoom) )
            iopar.get( KeyBindings::sZoom, keybind.zoom );
        if ( iopar.hasKey(KeyBindings::sRotate) )
            iopar.get( KeyBindings::sRotate, keybind.rotate );
        if ( iopar.hasKey(KeyBindings::sPan) )
            iopar.get( KeyBindings::sPan, keybind.pan );
        while ( !atEndOfSection(astrm) ) astrm.next();
        astrm.next();
        keyset += new KeyBindings( keybind );
    }

    mSettUse(get,"dTect.MouseControls","Default",curkeyb);
    setKeyBindings( curkeyb );
}


void KeyBindMan::setKeyBindings( const char* name )
{
    curkeyb = BufferString(name);
    mSettUse(set,"dTect.MouseControls","Default",curkeyb);
    Settings::common().write();

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
    zoom.mousebut = fms.size() ? fms[0] : KeyBindings::sRight;
    zoom.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone;

    res = keys.rotate;
    fms = FileMultiString(res);
    rotate.mousebut = fms.size() ? fms[0] : KeyBindings::sLeft;
    rotate.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone;

    res = keys.pan;
    fms = FileMultiString(res);
    pan.mousebut = fms.size() ? fms[0] : KeyBindings::sMiddle;
    pan.keybut = fms.size() > 1 ? fms[1] : KeyBindings::sNone;
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
                buttxt = KeyBindings::sLeft;
                break;
            case SoMouseButtonEvent::BUTTON2:
                buttxt = KeyBindings::sRight;
                break;
            case SoMouseButtonEvent::BUTTON3:
                buttxt = KeyBindings::sMiddle;
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

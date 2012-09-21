#ifndef keybindings_h
#define keybindings_h

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Dec 2002
 RCS:           $Id$
________________________________________________________________________

*/

#include "uicoinmod.h"
#include "bufstringset.h"
class SoMouseButtonEvent;
class SoEvent;

/*! \brief Class for setting keybindings.

Each binding is a BufferString. This string contains the several keys 
separated by a `.  e.g. zoom = "Left`Control"

*/


mClass(uiCoin) KeyBindings
{
public:
    				KeyBindings(const char* nm=0)
				    : name(nm) {};

    BufferString		name;

    BufferString		zoom;
    BufferString		rotate;
    BufferString		pan;


    static const char*		sName();
    static const char*		sRotate();
    static const char*		sPan();
    static const char*		sZoom();

    static const char*		sControl();
    static const char*		sShift();
    static const char*		sRight();
    static const char*		sLeft();
    static const char*		sMiddle();
    static const char*		sNone();
};


mClass(uiCoin) EventButton
{
public:
                                EventButton() {}

    BufferString                mousebut;
    BufferString                keybut;
};


mClass(uiCoin) KeyBindMan
{
public:
                                KeyBindMan();
                                ~KeyBindMan();

    void                        setKeyBindings(const char*,bool saveinsett);
    void                        getAllKeyBindings(BufferStringSet&);
    const char*			getCurrentKeyBindings() const
				{ return curkeyb; }

    const SoEvent*		processSoEvent(const SoEvent* const,bool,bool);

protected:

    bool                        correctButtonsPushed(EventButton,const char*);
    void                        doZoom(SoMouseButtonEvent*);
    void                        doPan(SoMouseButtonEvent*);
    void                        doRotate(SoMouseButtonEvent*);

    ObjectSet<KeyBindings>      keyset;
    BufferString		curkeyb;

    EventButton                 zoom;
    EventButton                 pan;
    EventButton                 rotate;

    bool                        dozoom;
    bool                        shiftpress;
    bool                        ctrlpress;
    bool                        useownevent;
};

#endif


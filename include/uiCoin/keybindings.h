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

/*!
\brief Class for setting keybindings.

  Each binding is a BufferString. This string contains the several keys 
  separated by a `.  e.g. zoom = "Left`Control"
*/

mExpClass(uiCoin) KeyBindings
{
public:
    				KeyBindings(const char* nm=0)
				    : name(nm) {};

    BufferString		name;

    BufferString		zoom;
    BufferString		rotate;
    BufferString		pan;


    static FixedString		sName();
    static FixedString		sRotate();
    static FixedString		sPan();
    static FixedString		sZoom();

    static FixedString		sControl();
    static FixedString		sShift();
    static FixedString		sRight();
    static FixedString		sLeft();
    static FixedString		sMiddle();
    static FixedString		sNone();
};


/*!
\brief Event button.
*/

mExpClass(uiCoin) EventButton
{
public:
                                EventButton() {}

    BufferString                mousebut;
    BufferString                keybut;
};


/*!
\brief Manages keybindings.
*/

mExpClass(uiCoin) KeyBindMan
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


#ifndef keybindings_h
#define keybindings_h

/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          Dec 2002
 RCS:           $Id: keybindings.h,v 1.3 2003-10-17 14:19:01 bert Exp $
________________________________________________________________________

*/

#include "bufstringset.h"
class SoMouseButtonEvent;
class SoEvent;

/*! \brief Class for setting keybindings.

Each binding is a BufferString. This string contains the several keys 
separated by a `.  e.g. zoom = "Left`Control"

*/


class KeyBindings
{
public:
    				KeyBindings(const char* nm=0)
				    : name(nm) {};

    BufferString		name;

    BufferString		zoom;
    BufferString		rotate;
    BufferString		pan;


    static const char*		sName;
    static const char*		sRotate;
    static const char*		sPan;
    static const char*		sZoom;

    static const char*		sControl;
    static const char*		sShift;
    static const char*		sRight;
    static const char*		sLeft;
    static const char*		sMiddle;
    static const char*		sNone;
};


class EventButton
{
public:
                                EventButton() {}

    BufferString                mousebut;
    BufferString                keybut;
};


class KeyBindMan
{
public:
                                KeyBindMan();

    void                        setKeyBindings(const char*);
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

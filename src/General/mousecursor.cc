/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2008
-*/

static const char* rcsID = "$Id: mousecursor.cc,v 1.2 2008-03-18 18:42:12 cvskris Exp $";

#include "mousecursor.h"


bool MouseCursor::operator==( const MouseCursor& mc ) const
{
    if ( mc.shape_!=shape_ )
	return false;

    if ( mc.shape_!=Bitmap )
	return true;

    return mc.filename_==filename_ && mc.hotx_==hotx_ && mc.hoty_==hoty_;
}


bool MouseCursor::operator!=( const MouseCursor& mc ) const
{ return !(*this==mc); }


PtrMan<MouseCursorManager> MouseCursorManager::mgr_ = 0;


void MouseCursorManager::setOverride( MouseCursor::Shape s, bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideShape( s, replace );
}


void MouseCursorManager::setOverride( const MouseCursor& mc, bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideCursor( mc, replace );
}


void MouseCursorManager::setOverride( const char* fnm, int hotx, int hoty,
				      bool replace )
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;
    mgr->setOverrideFile( fnm, hotx, hoty, replace );
}


void MouseCursorManager::restoreOverride()
{
    MouseCursorManager* mgr = MouseCursorManager::mgr();
    if ( !mgr ) return;

    mgr->restoreInternal();
}


MouseCursorManager* MouseCursorManager::mgr()
{ return MouseCursorManager::mgr_; }


void MouseCursorManager::setMgr( MouseCursorManager* mgr )
{
    MouseCursorManager::mgr_ = mgr;
}

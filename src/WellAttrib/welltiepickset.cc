/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.1 2009-04-28 14:30:26 cvsbruno Exp $";

#include "welltiepickset.h"

WellTiePickSetManager::WellTiePickSetManager()
	: CallBacker(CallBacker::CallBacker())
   	, mousepos_(0)
   	, pickadded(this)
	, mousemoving(this)		       
{
}


WellTiePickSetManager::~WellTiePickSetManager()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSetManager::updateShift( int vwridx, float curpos )
{
    mousepos_ = curpos;
    mousemoving.trigger();
}


void WellTiePickSetManager::addPick( int vwridx, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx;
    pick->zpos_ = zpos;
    pick->color_ = Color::DgbColor();
    pickset_ += pick;
    pickadded.trigger();
}


void WellTiePickSetManager::clearPick()
{
    delete ( pickset_.remove(pickset_.size()-1) );
}


void WellTiePickSetManager::clearAllPicks()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}

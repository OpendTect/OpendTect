/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.2 2009-05-15 12:42:48 cvsbruno Exp $";

#include "welltiepickset.h"

WellTiePickSetManager::WellTiePickSetManager()
	: CallBacker(CallBacker::CallBacker())
   	, mousepos_(0)
	, nrpickstotal_(0)	      
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
    nrpickstotal_++;
    pickadded.trigger();
}


void WellTiePickSetManager::clearPick( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSetManager::clearAllPicks()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSetManager::updateSupPickedPos( float& lastpos, float curpos,
       						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getPickPos(idx);
	if ( pos > curpos ) 
	{	
	    if ( (pos - curpos) < ( lastpos - curpos ) )
		lastpos = pos;
	}
    }
}


void WellTiePickSetManager::updateInfPickedPos( float& firstpos, float curpos,
						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getPickPos(idx);
	if ( pos < curpos )
	{
	    if ( (pos - curpos) > ( firstpos - curpos) )
		firstpos = pos;
	}
    }
}


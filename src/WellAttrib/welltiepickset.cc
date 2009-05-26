/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.4 2009-05-26 07:06:53 cvsbruno Exp $";

#include "welltiepickset.h"

WellTiePickSetMGR::WellTiePickSetMGR()
	: CallBacker(CallBacker::CallBacker())
   	, pickadded(this)
	, mousemoving(this)		       
{
}

WellTiePickSetMGR::~WellTiePickSetMGR()
{}


void WellTiePickSetMGR::addPick( int vwridx, float zpos )
{
    if ( vwridx == 0 )
	logpickset_.add( vwridx, zpos );

    else if ( abs(seispickset_.getSize() - synthpickset_.getSize()) < 2 )
    {
	if ( vwridx == 4 )
	    synthpickset_.add( vwridx, zpos );
	else if ( vwridx == 5 )
	    seispickset_.add( vwridx, zpos );
    }
    else return;

    pickadded.trigger(vwridx);
}


void WellTiePickSetMGR::updateShift( int vwridx, float curpos )
{
    logpickset_.setMousePos( curpos );
    mousemoving.trigger();
}


void WellTiePickSetMGR::clearAllPicks()
{
    logpickset_.clearAll();
    seispickset_.clearAll();
    synthpickset_.clearAll();
}



WellTiePickSet::WellTiePickSet()
	: mousepos_(0)
	, nrpickstotal_(0)
{
}


WellTiePickSet::~WellTiePickSet()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::add( int vwridx, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx;
    pick->zpos_ = zpos;
    pick->color_ = Color::DgbColor();
    pickset_ += pick;
    nrpickstotal_++;
}


void WellTiePickSet::clear( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::clearAll()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::updateSupPickedPos( float& lastpos, float curpos,
       						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getPos(idx);
	if ( pos > curpos ) 
	{	
	    if ( (pos - curpos) < ( lastpos - curpos ) )
		lastpos = pos;
	}
    }
}


void WellTiePickSet::updateInfPickedPos( float& firstpos, float curpos,
						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getPos(idx);
	if ( pos < curpos )
	{
	    if ( (pos - curpos) > ( firstpos - curpos) )
		firstpos = pos;
	}
    }
}


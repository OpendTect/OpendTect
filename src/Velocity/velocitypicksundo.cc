/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "velocitypicksundo.h"
#include "velocitypicks.h"

namespace Vel
{

PickSetEvent::PickSetEvent( Picks& picks, const Pick& oldpick,
					  const Pick& newpick,
					  const BinID& bid)
    : picks_( picks )
    , oldpick_( oldpick )
    , newpick_( newpick )
    , bid_( bid )
{ }


bool PickSetEvent::unDo()
{
    const RowCol pos = picks_.find( bid_, newpick_.depth_ );
    if ( pos.row<0 && pos.col<0 )
	return false;

    picks_.set( pos, oldpick_, false );
    return true;
}


    
bool PickSetEvent::reDo()
{
    const RowCol pos = picks_.find( bid_, oldpick_.depth_ );
    if ( pos.row<0 && pos.col<0 )
	return false;

    picks_.set( pos, newpick_, false );
    return true;
}


const char* PickSetEvent::getStandardDesc() const
{ return "changed velocity pick"; }



PickAddEvent::PickAddEvent( Picks& picks, const RowCol& pos )
    : picks_( picks )
    , newpick_( mUdf(float), mUdf(float), -1 )
    , newbid_( -1, -1 )
{
    picks_.get( pos, newbid_, newpick_ );
}


bool PickAddEvent::unDo()
{
    const RowCol pos = picks_.find( newbid_, newpick_.depth_ );
    if ( pos.row<0 && pos.col<0 )
	return false;

    picks_.remove( pos, false );
    return true;
}

    
bool PickAddEvent::reDo()
{
    picks_.set( newbid_, newpick_, false );
    return true;
}


const char* PickAddEvent::getStandardDesc() const
{ return "new velocity pick"; }


PickRemoveEvent::PickRemoveEvent( Picks& picks,
	const RowCol& pos )
    : picks_( picks )
    , oldpick_( mUdf(float), mUdf(float), -1 )
    , oldbid_( -1, -1 )
{
    picks_.get( pos, oldbid_, oldpick_ );
}


bool PickRemoveEvent::reDo()
{
    const RowCol pos = picks_.find( oldbid_, oldpick_.depth_ );
    if ( pos.row<0 && pos.col<0 )
	return false;

    picks_.remove( pos, false );
    return true;
}

    
bool PickRemoveEvent::unDo()
{
    picks_.set( oldbid_, oldpick_, false );
    return true;
}


const char* PickRemoveEvent::getStandardDesc() const
{ return "removed Velocity pick"; }


};//namespace

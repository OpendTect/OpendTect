/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/


#include "geomelement.h"
#include "survinfo.h"
#include "uistrings.h"

namespace Geometry
{
Element::Element()
    : nrpositionnotifier( this )
    , movementnotifier( this )
    , ischanged_( false )
    , errmsg_( 0 )
    , blockcbs_( false )
{ }


Element::~Element()
{ delete errmsg_; }


void Element::getPosIDs( TypeSet<GeomPosID>& res, bool noudf ) const
{
    PtrMan<Iterator> iter = createIterator();

    GeomPosID posid;
    while ( (posid=iter->next())!=-1 )
    {
	if ( noudf && !isDefined( posid ) )
	    continue;

	res+= posid;
    }
}


IntervalND<float> Element::boundingBox(bool) const
{
    IntervalND<float> bbox(3);
    Coord3 pos;

    PtrMan<Iterator> iter = createIterator();
    GeomPosID posid;
    while ( (posid=iter->next())!=-1 )
    {
	pos = getPosition( posid );

	if ( !bbox.isSet() ) bbox.setRange(pos);
	else bbox.include(pos);
    }

    return bbox;
}



uiString Element::errMsg() const
{ return errmsg_ ? *errmsg_ : uiString::emptyString(); }


uiString& Element::errmsg()
{
    if ( !errmsg_ ) errmsg_ = new uiString;
    return *errmsg_;
}


void Element::blockCallBacks( bool yn, bool flush )
{
    if ( flush )
    {
	blockcbs_ = false;
	triggerNrPosCh( nrposchbuffer_ );
	triggerMovement( movementbuffer_ );
    }

    blockcbs_ = yn;

    if ( blockcbs_ && !flush )
	return;

    Threads::Locker poschglocker( poschglock_ );
    nrposchbuffer_.erase();
    poschglocker.unlockNow();

    Threads::Locker movementlocker( movementlock_ );
    movementbuffer_.erase();
    movementlocker.unlockNow();
}


void Element::triggerMovement( const TypeSet<GeomPosID>& gpids )
{
    if ( gpids.isEmpty() )
	return;

    if ( blockcbs_ )
    {
	Threads::Locker locker( movementlock_ );
	movementbuffer_.append( gpids );
    }
    else
	movementnotifier.trigger( &gpids, this );

    ischanged_ = true;
}


void Element::triggerMovement( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    triggerMovement( gpids );
}


void Element::triggerMovement()
{
    if ( blockcbs_ )
	getPosIDs( movementbuffer_, true );
    else
	movementnotifier.trigger( 0, this );

    ischanged_ = true;
}


void Element::triggerNrPosCh( const TypeSet<GeomPosID>& gpids )
{
    if ( gpids.isEmpty() ) return;

    if ( blockcbs_ )
    {
	Threads::Locker locker( poschglock_ );
	nrposchbuffer_.append( gpids );
    }
    else
	nrpositionnotifier.trigger( &gpids, this );

    ischanged_ = true;
}


void Element::triggerNrPosCh( const GeomPosID& gpid )
{
    TypeSet<GeomPosID> gpids( 1, gpid );
    triggerNrPosCh( gpids );
}


void Element::triggerNrPosCh()
{
    if ( blockcbs_ )
	getPosIDs( nrposchbuffer_, true );
    else
	nrpositionnotifier.trigger( 0, this );
    ischanged_ = true;
}

} // namespace Geometry

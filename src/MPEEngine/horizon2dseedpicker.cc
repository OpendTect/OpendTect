/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon2dseedpicker.cc,v 1.1 2006-05-05 19:07:54 cvskris Exp $";

#include "horizon2dseedpicker.h"

#include "attribdataholder.h"
//#include "autotracker.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "seisinfo.h"
//#include "sectiontracker.h"
//#include "executor.h"
//#include "mpeengine.h"
//#include "survinfo.h"
//#include "sorting.h"


namespace MPE 
{

Horizon2DSeedPicker::Horizon2DSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , data2d_( 0 )
{ }


Horizon2DSeedPicker::~Horizon2DSeedPicker()
{
    if ( data2d_ ) data2d_->unRef();
}


void Horizon2DSeedPicker::setLine( const MultiID& lineset, const char* linename,
				   const Attrib::Data2DHolder* data )
{
     if ( data2d_ )
	 data2d_->unRef();

     data2d_ = data;
     if ( data2d_ ) data2d_->ref();

     lineset_ = lineset;
     linename_ = linename;
}


bool Horizon2DSeedPicker::canSetSectionID() const
{ return true; }


bool Horizon2DSeedPicker::setSectionID( const EM::SectionID& sid )
{ 
    sectionid_ = sid; 
    return true; 
}


EM::SectionID Horizon2DSeedPicker::getSectionID() const
{ return sectionid_; }


#define mGetHorizon(hor) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet( EM::Horizon2D*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return false;\


bool Horizon2DSeedPicker::startSeedPick()
{
    if ( !data2d_ ) return false;

    mGetHorizon(hor);
    didchecksupport_ = hor->enableGeometryChecks( false );

    bool found = false;
    for ( int idx=0; idx<hor->geometry().nrLines(); idx++ )
    {
	const int lineid = hor->geometry().lineID( idx );
	if ( hor->geometry().lineSet(lineid)==lineset_ &&
	     !strcmp(hor->geometry().lineName(lineid),linename_) )
	{
	    found = true;
	    lineid_ = lineid;
	    break;
	}
    }

    if ( !found )
    {
	TypeSet<Coord> coords;
	const CubeSampling cs = data2d_->getCubeSampling();
	const StepInterval<int> trcrg( cs.hrg.start.crl, cs.hrg.stop.crl,
				       cs.hrg.step.crl );

	for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+= trcrg.step )
	{
	    found = false;
	    for ( int idx=0; idx<data2d_->size(); idx++ )
	    {
		if ( data2d_->trcinfoset_[idx]->nr == trcnr )
		{
		    coords += data2d_->trcinfoset_[idx]->coord;
		    found = true;
		    break;
		}
	    }

	    if ( !found ) coords += Coord( mUdf(float), mUdf(float) );
	}

	lineid_ = hor->geometry().addLine( coords, trcrg.start, trcrg.step,
					   lineset_, linename_ );
    }

    return true;
}

	
bool Horizon2DSeedPicker::addSeed(const Coord3& seedcrd )
{
    mGetHorizon(hor);

    const StepInterval<int> colrg =
	hor->geometry().colRange( sectionid_, lineid_ );

    if ( colrg.start==colrg.stop )
	return false;

    float maxdist = mUdf(float);
    int   closestcol;
    RowCol rc( lineid_, 0 );
    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
    {
	const Coord coord = hor->getPos( sectionid_, rc.getSerialized() );
	if ( !coord.isDefined() )
	    continue;

	double sqdist = coord.sqDistance( seedcrd );
	if ( sqdist<maxdist )
	{
	    closestcol = rc.col;
	    maxdist = sqdist;
	}
    }

    rc.col = closestcol;
	
    const EM::PosID pid( hor->id(), sectionid_, rc.getSerialized() );

    hor->setPos( pid, seedcrd, true );
    if ( !hor->isPosAttrib( pid, EM::EMObject::sSeedNode ) )
	hor->setPosAttrib( pid, EM::EMObject::sSeedNode, true );

    return reTrack();
}


bool Horizon2DSeedPicker::doesModeUseSetup() const
{ return true; }


bool Horizon2DSeedPicker::reTrack()
{ return true; }


int Horizon2DSeedPicker::getSeedConnectMode() const
{ return 0; }


void Horizon2DSeedPicker::blockSeedPick(bool)
{}


int Horizon2DSeedPicker::isMinimumNrOfSeeds() const
{ return 0; }


void Horizon2DSeedPicker::setSeedConnectMode(int)
{}


bool Horizon2DSeedPicker::isSeedPickBlocked() const
{ return false; }


bool Horizon2DSeedPicker::doesModeUseVolume() const
{ return false; }


bool Horizon2DSeedPicker::stopSeedPick(bool)
{ return true; }


bool Horizon2DSeedPicker::removeSeed(EM::PosID const&, bool)
{ return true; }


int MPE::Horizon2DSeedPicker::nrSeeds() const
{ return 0; }



}; // namespace MPE


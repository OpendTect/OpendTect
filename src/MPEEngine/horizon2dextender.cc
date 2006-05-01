/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2006
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon2dextender.cc,v 1.1 2006-05-01 17:26:17 cvskris Exp $";

#include "horizon2dextender.h"

#include "emhorizon2d.h"
#include "survinfo.h"

namespace MPE 
{

Horizon2DExtender::Horizon2DExtender( EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionExtender( sid )
    , surface_( hor )
    , anglethreshold_( 0.5 )
{}


void Horizon2DExtender::setAngleThreshold( float rad )
{ anglethreshold_ = cos( rad ); }


float Horizon2DExtender::getAngleThreshold() const
{ return acos(anglethreshold_); }


void Horizon2DExtender::setDirection( const BinIDValue& dir )
{
    direction_ = dir;
    xydirection_ = SI().transform( BinID(0,0) ) - SI().transform( dir.binid );
    const double abs = xydirection_.abs();
    alldirs_ = mIsZero( abs, 1e-3 );
    if ( !alldirs_ )
	xydirection_ /= abs;
}


int Horizon2DExtender::nextStep()
{
    const bool alldirs = direction_.binid.inl==0 && direction_.binid.crl==0;

    for ( int idx=0; idx<startpos_.size(); idx++ )
    {
	const RowCol rc( startpos_[idx] );
	const StepInterval<int> colrange =
	    surface_.geometry().colRange( sid_, rc.row );
	const Coord3 startpos = surface_.getPos( sid_, startpos_[idx] );

	if ( rc.col!=colrange.start )
	    addPos( false, rc, startpos, colrange.step );
	if ( rc.col!=colrange.stop )
	    addPos( true, rc, startpos, colrange.step );
    }

    return 0;
}


void Horizon2DExtender::addPos( bool next, const RowCol& rc,
				const Coord& startpos, int step )
{
    const EM::SubID neighborsubid =
	RowCol(rc.row,next ? rc.col+step : rc.col-step).getSerialized();

    if ( !alldirs_ )
    {
	const Coord neighborpos = surface_.getPos( sid_, neighborsubid );
	const Coord dir = neighborpos - startpos;
	const double dirabs = dir.abs();
	if ( !mIsZero(dirabs,1e-3) )
	{
	    const Coord normdir = dir/dirabs;
	    const double cosangle = normdir.dot(xydirection_);
	    if ( cosangle<anglethreshold_ )
		return;
	}
    }

    addTarget( neighborsubid, rc.getSerialized() );
}


};  // namespace MPE

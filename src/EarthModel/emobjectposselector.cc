/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2008
________________________________________________________________________

-*/

#include "emobjectposselector.h"

#include "arraynd.h"
#include "binidsurface.h"
#include "emobject.h"
#include "parametricsurface.h"
#include "survinfo.h"

namespace EM
{

EMObjectPosSelector::EMObjectPosSelector( const EMObject& emobj,
				const ObjectSet<const Selector<Coord3> >& sel )
    : ParallelTask()
    , emobj_(emobj)
    , selectors_(sel)
    , startrow_(-1)
    , nrrows_(-1)
    , startcol_(-1)
    , nrcols_(-1)
{
    emobj_.ref();
}


EMObjectPosSelector::~EMObjectPosSelector()
{ emobj_.unRef(); }


bool EMObjectPosSelector::doPrepare( int nrthreads )
{
    poslist_.erase();
    //TODO this is temporary extraction way of z values
    //this will get replaced by well though class structue

    const Geometry::Element* ge = emobj_.geometryElement();
    mDynamicCastGet(const Geometry::BinIDSurface*,surf,ge);
    if ( !surf || !surf->getArray() )
	return false;

    startrow_ = surf->rowRange().start;
    nrrows_ = surf->rowRange().nrSteps() +1 ;
    startcol_ = surf->colRange().start;
    nrcols_ = surf->colRange().nrSteps() + 1 ;

    zvals_ = surf->getArray()->getData();

    starts_.erase();
    stops_.erase();

    starts_ += RowCol(startrow_, startcol_);
    stops_ += RowCol( startrow_+(nrrows_-1)*surf->rowRange().step,
		      startcol_+(nrcols_-1)*surf->colRange().step );

    finished_ = false;
    nrwaiting_ = 0;
    nrthreads_ = nrthreads;

    return true;
}


bool EMObjectPosSelector::doWork( od_int64, od_int64, int threadid )
{
    lock_.lock();

    while( !finished_ )
    {
	if ( !starts_.size() )
	{
	    nrwaiting_ ++;
	    if ( nrwaiting_==nrthreads_ )
	    {
		finished_ = true;
		lock_.signal( true );
		nrwaiting_--;
		break;
	    }

	    lock_.wait();
	    nrwaiting_--;
	}

	const int sz = starts_.size();
	if ( !sz )
	    continue;

	RowCol start = starts_[sz-1];
	RowCol stop = stops_[sz-1];

	starts_.removeSingle( sz-1 );
	stops_.removeSingle( sz-1 );

	lock_.unLock();

	processBlock( start, stop );

	lock_.lock();
    }

    lock_.unLock();

    return true;
}


void EMObjectPosSelector::processBlock( const RowCol& start,
					const RowCol& stop )
{
    const Geometry::Element* ge = emobj_.geometryElement();
    mDynamicCastGet(const Geometry::BinIDSurface*,surf,ge);
    if ( !surf ) return;

    const int rowstep = surf->rowRange().step;
    const int colstep = surf->colRange().step;

    Coord3 up = Coord3::udf();
    Coord3 down = Coord3::udf();

    getBoundingCoords( start, stop, up, down );

    for ( int sidx=0; sidx<selectors_.size(); sidx++ )
    {
	const Selector<Coord3>* selector = selectors_[sidx];
	const int sel = !selector->canDoRange() ? 1
				    : selector->includesRange( up, down );
	if ( sel==0 || sel==3 )
	    continue; // all outside or all behind projection plane

	int rowlen = (stop.row()-start.row()) / rowstep;
	int collen = (stop.col()-start.col()) / colstep;

	if ( rowlen < 32 && collen < 32 )
	    makeListGrow( start, stop, sel );
	else if ( rowlen < 32 && collen >= 32 )
	{
	    lock_.lock();

	    starts_ += start;
	    stops_ += RowCol( stop.row(), start.col()+colstep*(collen/2) );

	    lock_.signal( starts_.size()>1 );

	    lock_.unLock();

	    processBlock( RowCol(start.row(),start.col()+colstep*(1+collen/2)),
			  stop );
	}
	else if ( rowlen >=32 && collen < 32 )
	{
	    lock_.lock();

	    starts_ += start;
	    stops_ += RowCol( start.row()+rowstep*(rowlen/2), stop.col() );

	    lock_.signal( starts_.size()>1 );

	    lock_.unLock();

	    processBlock( RowCol(start.row()+rowstep*(1+rowlen/2),start.col()),
			  stop );
	}
	else
	{
	    lock_.lock();

	    starts_ += start;
	    stops_ += RowCol( start.row()+rowstep*(rowlen/2),
			      start.col()+colstep*(collen/2) );

	    starts_ += RowCol( start.row(), start.col()+colstep*(1+collen/2) );
	    stops_ += RowCol( start.row()+rowstep*(rowlen/2), stop.col() );

	    starts_ += RowCol( start.row()+rowstep*(1+rowlen/2), start.col() );
	    stops_ += RowCol( stop.row(), start.col()+colstep*(collen/2) );

	    lock_.signal( starts_.size()>1 );

	    lock_.unLock();

	    processBlock( RowCol(start.row()+rowstep*(1+rowlen/2),
				 start.col()+colstep*(1+collen/2) ), stop );
	}
    }
}


void EMObjectPosSelector::getBoundingCoords( const RowCol& start,
						  const RowCol& stop,
						  Coord3& up, Coord3& down )
{
    const Geometry::Element* ge = emobj_.geometryElement();
    mDynamicCastGet(const Geometry::BinIDSurface*,surf,ge);
    if ( !surf ) return;

    const int rowstep = surf->rowRange().step;
    const int colstep = surf->colRange().step;

    Coord coord0 = SI().transform( BinID(start.row(),start.col()) );
    up.x = down.x = coord0.x;
    up.y = down.y = coord0.y;

    Coord coord1 = SI().transform( BinID(start.row(),stop.col()) );
    if ( up.x < coord1.x ) up.x = coord1.x;
    if ( up.y < coord1.y ) up.y = coord1.y;
    if ( coord1.x < down.x ) down.x = coord1.x;
    if ( coord1.y < down.y ) down.y = coord1.y;

    Coord coord2 = SI().transform( BinID(stop.row(),start.col()) );
    if ( up.x < coord2.x ) up.x = coord2.x;
    if ( up.y < coord2.y ) up.y = coord2.y;
    if ( coord2.x < down.x ) down.x = coord2.x;
    if ( coord2.y < down.y ) down.y = coord2.y;

    Coord coord3 = SI().transform( BinID(stop.row(),stop.col()) );
    if ( up.x < coord3.x ) up.x = coord3.x;
    if ( up.y < coord3.y ) up.y = coord3.y;
    if ( coord3.x < down.x ) down.x = coord3.x;
    if ( coord3.y < down.y ) down.y = coord3.y;

    up.z = down.z = mUdf(float);

    for ( int row=start.row(); row<=stop.row(); row+=rowstep )
    {
	int idx = nrcols_*(row-startrow_) /
				rowstep+(start.col()-startcol_)/colstep;
	for ( int col=start.col(); col<=stop.col(); col+=colstep, idx++ )
	{
	    const float val = zvals_[idx];
	    if ( mIsUdf(val) )
		continue;
	    if ( mIsUdf(up.z) || val>up.z )
		up.z = val;
	    if ( mIsUdf(down.z) || val<down.z )
		down.z = val;
	}
    }
}


void EMObjectPosSelector::makeListGrow( const RowCol& start,
					const RowCol& stop, int selresult )
{
    const Geometry::Element* ge = emobj_.geometryElement();
    if ( !ge ) return;

     mDynamicCastGet(const Geometry::RowColSurface*,surf,ge);
     if ( !surf ) return;

    const StepInterval<int> rowrg( start.row(), stop.row(),
				   surf->rowRange().step );
    const StepInterval<int> colrg( start.col(), stop.col(),
				   surf->colRange().step );

    TypeSet<EM::SubID> ids;

    TrcKeySampling trcsampling( true );
    trcsampling.set( rowrg, colrg );
    TrcKeySamplingIterator iter( trcsampling );

    BinID bid;

    iter.reset();
    while( iter.next(bid) )
    {
	if ( selresult != 2 )     // not all inside
	{
	    const Coord3 crd = emobj_.getPos( bid.toInt64() );
	    if ( !crd.isDefined() ) continue;

	    bool found = false;
	    for ( int sidx=0; sidx<selectors_.size(); sidx++ )
	    {
		const Selector<Coord3>* sel = selectors_[sidx];
		if ( sel && sel->includes(crd) )
		{
		    found = true;
		    break;
		}
	    }

	    if ( !found ) continue;
	}

	ids += bid.toInt64();
    }

    lock_.lock();
    poslist_.append( ids );
    lock_.unLock();
}

} // namespace EM

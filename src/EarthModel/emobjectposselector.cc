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

ObjectPosSelector::ObjectPosSelector( const Object& emobj,
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


ObjectPosSelector::~ObjectPosSelector()
{ emobj_.unRef(); }


bool ObjectPosSelector::doPrepare( int nrthreads )
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


bool ObjectPosSelector::doWork( od_int64, od_int64, int threadid )
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


void ObjectPosSelector::processBlock( const RowCol& start,
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


void ObjectPosSelector::getBoundingCoords( const RowCol& start,
						  const RowCol& stop,
						  Coord3& up, Coord3& down )
{
    const Geometry::Element* ge = emobj_.geometryElement();
    mDynamicCastGet(const Geometry::BinIDSurface*,surf,ge);
    if ( !surf ) return;

    const int rowstep = surf->rowRange().step;
    const int colstep = surf->colRange().step;

    Coord coord0 = SI().transform( BinID(start.row(),start.col()) );
    up.x_ = down.x_ = coord0.x_;
    up.y_ = down.y_ = coord0.y_;

    Coord coord1 = SI().transform( BinID(start.row(),stop.col()) );
    if ( up.x_ < coord1.x_ ) up.x_ = coord1.x_;
    if ( up.y_ < coord1.y_ ) up.y_ = coord1.y_;
    if ( coord1.x_ < down.x_ ) down.x_ = coord1.x_;
    if ( coord1.y_ < down.y_ ) down.y_ = coord1.y_;

    Coord coord2 = SI().transform( BinID(stop.row(),start.col()) );
    if ( up.x_ < coord2.x_ ) up.x_ = coord2.x_;
    if ( up.y_ < coord2.y_ ) up.y_ = coord2.y_;
    if ( coord2.x_ < down.x_ ) down.x_ = coord2.x_;
    if ( coord2.y_ < down.y_ ) down.y_ = coord2.y_;

    Coord coord3 = SI().transform( BinID(stop.row(),stop.col()) );
    if ( up.x_ < coord3.x_ ) up.x_ = coord3.x_;
    if ( up.y_ < coord3.y_ ) up.y_ = coord3.y_;
    if ( coord3.x_ < down.x_ ) down.x_ = coord3.x_;
    if ( coord3.y_ < down.y_ ) down.y_ = coord3.y_;

    up.z_ = down.z_ = mUdf(float);

    for ( int row=start.row(); row<=stop.row(); row+=rowstep )
    {
	int idx = nrcols_*(row-startrow_) / rowstep +
		  (start.col()-startcol_) / colstep;
	for ( int col=start.col(); col<=stop.col(); col+=colstep, idx++ )
	{
	    const float val = zvals_[idx];
	    if ( mIsUdf(val) )
		continue;
	    if ( mIsUdf(up.z_) || val>up.z_ )
		up.z_ = val;
	    if ( mIsUdf(down.z_) || val<down.z_ )
		down.z_ = val;
	}
    }
}


void ObjectPosSelector::makeListGrow( const RowCol& start,
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

    TypeSet<EM::PosID> ids;

    TrcKeySampling trcsampling( true );
    trcsampling.set( rowrg, colrg );
    TrcKeySamplingIterator iter( trcsampling );
    do
    {
	const TrcKey trk( iter.curTrcKey() );
	const EM::PosID posid( PosID::getFromRowCol(trk.position()) );
	if ( selresult != 2 )     // not all inside
	{
	    const Coord3 crd = emobj_.getPos( posid );
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

	ids += posid;
    } while ( iter.next() );

    lock_.lock();
    poslist_.append( ids );
    lock_.unLock();
}

} // namespace EM

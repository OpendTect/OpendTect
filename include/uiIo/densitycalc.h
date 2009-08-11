/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: densitycalc.h,v 1.1 2009-08-11 07:21:23 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"

#include "uidatapointset.h"
#include "uiworld2ui.h"

#include "arraynd.h"
#include "mathexpression.h"
#include "polygon.h"
#include "rowcol.h"
#include "task.h"
#include "thread.h"

class DensityCalc : public ParallelTask
{
public:
DensityCalc( uiDataPointSet& uidps, Array2D<float>* data,
	     uiDataPointSetCrossPlotter::AxisData& x,
	     uiDataPointSetCrossPlotter::AxisData& y,
	     const ObjectSet<uiDataPointSetCrossPlotter::SelectionArea>& areas,
	     const char* header )
    : ParallelTask( header )
    , uidps_( uidps )
    , dps_( uidps.pointSet() )
    , selareaset_( areas )
    , data_( data )
    , x_( x )
    , y_( y )
    , mathobj_( 0 )
    , xpixrg_( x_.axis_->pixRange() )
    , ypixrg_( y_.axis_->pixRange() )
    , changedps_( false )
    , removesel_( false )
    , curgrp_( 0 )
    , nrdone_( 0 )
    , indexsz_( 0 )
{}

od_int64 nrDone() const { return nrdone_; }
od_int64 nrIterations() const { return dps_.size(); }

bool isSelectionValid( uiDataPointSet::DRowID rid )
{
    if ( modcolidxs_.size() && mathobj_ )
    {
	for ( int idx=0; idx<modcolidxs_.size(); idx++ )
	{
	    const float yval = uidps_.getValue( modcolidxs_[idx], rid, true );
	    mathobj_->setVariableValue( idx, yval );
	}

	const float result = mathobj_->getValue();
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 rid=start; rid<=stop; rid++ )
    {
	nrdone_ = rid;
	if ( dps_.isInactive(rid) || (curgrp_>0 && dps_.group(rid)!=curgrp_) )
	    continue;

	const float xval = uidps_.getValue( x_.colid_, rid, true );
	const float yval = uidps_.getValue( y_.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	uiWorldPoint wpt( (double)x_.axis_->getPix(xval),
			  (double)y_.axis_->getPix(yval) );
	uiPoint pos( x_.axis_->getPix(xval), y_.axis_->getPix(yval) );
	uiPoint datapt = w2ui_.transform( wpt );
	
	if ( !xpixrg_.includes(pos.x) || !ypixrg_.includes(pos.y) )
	    continue;

	bool ptremoved = false;
	if ( selareaset_.size() && (changedps_ || removesel_) )
	{
	    for ( int idx=0; idx<selareaset_.size(); idx++ )
	    {
		const uiDataPointSetCrossPlotter::SelectionArea* selarea =
		    selareaset_.size() ? selareaset_[idx] : 0;
		if ( !selarea )
		    continue;

		if ( selarea->isInside(pos) )
		{
		    Threads::MutexLocker lock( mutex_ );
		    if ( !isSelectionValid(rid) )
			continue;
		    if ( removesel_ )
		    {
			dps_.setInactive( rid, true );
			ptremoved = true;
		    }
		    else
		    {
			dps_.setSelected( rid, true );
			RowCol rcol( uidps_.tRowID(rid),
				     uidps_.tColID(y_.colid_) );
			selrowcols_ += rcol;
		    }
		}
		else
		    dps_.setSelected( rid, false );
	    }
	}

	if ( !data_->info().validPos(datapt.x,datapt.y) )
	    continue;
	data_->set( datapt.x, datapt.y,
		   ptremoved ? 0 : data_->get(datapt.x,datapt.y) + (float)1 );
	if ( indexsz_ < mNINT(data_->get(datapt.x,datapt.y)) )
	    indexsz_ = mNINT(data_->get(datapt.x,datapt.y));
    }
    return true;
}

void setWorld2Ui( const uiWorld2Ui& w2ui )	{ w2ui_ = w2ui; }

void setMathObj( MathExpression* mathobj )	{ mathobj_ = mathobj; }

void setModifiedColIds( const TypeSet<uiDataPointSet::DColID>& colids )
{ modcolidxs_ = colids; }

void setDPSChangeable( bool changedps )	{ changedps_ = changedps; }

void setRemSelected( bool removesel )	{ removesel_ = removesel; }

void setCurGroup( int curgrp )		{ curgrp_ = curgrp; }

int indexSize() const			{ return indexsz_; }

const TypeSet<RowCol>& selRCs() const	{ return selrowcols_; }

protected:
    uiDataPointSet&			uidps_;
    DataPointSet&			dps_;
    MathExpression*			mathobj_;
    const ObjectSet<uiDataPointSetCrossPlotter::SelectionArea>&	selareaset_;
    uiWorld2Ui				w2ui_;
    TypeSet<RowCol>			selrowcols_;
    TypeSet<uiDataPointSet::DColID>	modcolidxs_;
    Array2D<float>*			data_;
    uiDataPointSetCrossPlotter::AxisData& x_;
    uiDataPointSetCrossPlotter::AxisData& y_;
    Interval<int>			xpixrg_;
    Interval<int>			ypixrg_;
    Threads::Mutex			mutex_;
    bool				changedps_;
    bool				removesel_;
    int					curgrp_;
    int					indexsz_;
    int					nrdone_;
};


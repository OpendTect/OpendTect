/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id$
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
#include <iostream>

class DensityCalc : public ParallelTask
{
public:
DensityCalc( uiDataPointSet& uidps, Array2D<float>* data,
	     uiDataPointSetCrossPlotter::AxisData& x,
	     uiDataPointSetCrossPlotter::AxisData& y,
	     const ObjectSet<SelectionGrp>& grps,
	     const char* header )
    : ParallelTask( header )
    , uidps_( uidps )
    , dps_( uidps.pointSet() )
    , selgrpset_( grps )
    , data_( data )
    , freqdata_( 0 )
    , x_( x )
    , y_( y )
    , mathobj_( 0 )
    , xpixrg_( x_.axis_->pixRange() )
    , ypixrg_( y_.axis_->pixRange() )
    , usedxpixrg_( Interval<int> (0,0) )
    , changedps_( false )
    , removesel_( false )
    , curgrp_( 0 )
    , nrdone_( 0 )
    , indexsz_( 0 )
    , cellxsize_( 1.0 )
    , cellysize_( 1.0 )
    , areatype_( 0 )
{
    if ( data_ )
    {
	const int celldatawdth = data_->info().getSize(0)%mNINT32(cellxsize_)
			    ? data_->info().getSize(0)/mNINT32(cellxsize_) + 1
			    : data_->info().getSize(0)/mNINT32(cellxsize_);
	const int celldataheight = data_->info().getSize(1)%mNINT32(cellysize_)
			    ? data_->info().getSize(1)/mNINT32(cellysize_) + 1
			    : data_->info().getSize(1)/mNINT32(cellysize_);
	freqdata_ = new Array2DImpl<float>( celldatawdth, celldataheight );
	freqdata_->setAll( (float)0 );
    }
}

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


void setNrBins( int nrbinx, int nrbiny )
{
    if ( !freqdata_ )
	freqdata_ = new Array2DImpl<float>( nrbinx, nrbiny );
    else
    {
	mDynamicCastGet(Array2DImpl<float>*,arrimpl,freqdata_)
	arrimpl->setSize( nrbinx, nrbiny );
    }

    freqdata_->setAll( (float)0 );
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( DataPointSet::RowID rid=mCast(DataPointSet::RowID, start); 
							rid<=stop; rid++ )
    {
	nrdone_++;
	if ( dps_.isInactive(rid) || (curgrp_>0 && dps_.group(rid)!=curgrp_) )
	    continue;

	const float xval = uidps_.getValue( x_.colid_, rid, true );
	const float yval = uidps_.getValue( y_.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	uiWorldPoint wpt( (double)x_.axis_->getPix(xval),
			  (double)y_.axis_->getPix(yval) );
	uiPoint pos( x_.axis_->getPix(xval), y_.axis_->getPix(yval) );
	uiPoint datapt = w2ui_.transform( wpt );
	
	if ( !xpixrg_.includes(pos.x,true) || !ypixrg_.includes(pos.y,true) )
	    continue;

	if ( usedxpixrg_.width() == 0 && usedxpixrg_.start == 0 )
	    usedxpixrg_ = Interval<int>( pos.x, pos.x );
	else
	    usedxpixrg_.include( pos.x );

	bool ptselected = false;
	bool ptremoved = false;
	if ( selgrpset_.size() && (changedps_ || removesel_) )
	{
	    for ( int idx=0; idx<selgrpset_.size(); idx++ )
	    {
		const SelectionGrp* selgrp = selgrpset_[idx];
		for ( int selidx=0; selidx<selgrp->size(); selidx++ )
		{
		    if ( !selgrp->isValidIdx(selidx) ) continue;
		    const SelectionArea& selarea =
			    selgrp->getSelectionArea( selidx );

		    if ( selarea.isInside(pos) )
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
			    ptselected = true;
			    dps_.setSelected( rid, uidps_.getSelectionGroupIdx(
					      selarea.id_) );
			    RowCol rcol( uidps_.tRowID(rid),
					 uidps_.tColID(y_.colid_) );
			    selrowcols_ += rcol;
			}
		    }
		}
	    }
	}

	if ( !ptselected )
	    dps_.setSelected( rid, -1 );
	if ( !data_->info().validPos(datapt.x,datapt.y) )
	    continue;
	const int freqx = mNINT32(datapt.x/cellxsize_);
	const int freqy = mNINT32(datapt.y/cellysize_);
	if ( !areatype_ )
	    freqdata_->set( freqx, freqy,
		    ptremoved ? 0 : freqdata_->get(freqx,freqy) + (float)1 );
	else if ( areatype_==1 && ptselected )
	    freqdata_->set( freqx, freqy,
		    ptremoved ? 0 : freqdata_->get(freqx,freqy) + (float)1 );
	else if ( areatype_==2 && !ptselected )
	    freqdata_->set( freqx, freqy,
		    ptremoved ? 0 : freqdata_->get(freqx,freqy) + (float)1 );

	if ( data_ )
	{
	    for ( int idx=0; idx<mNINT32(cellxsize_); idx++ )
	    {
		for ( int idy=0; idy<mNINT32(cellysize_); idy++ )
		    data_->set( freqx*mNINT32(cellxsize_)+idx,
			    	freqy*mNINT32(cellysize_)+idy,
				ptremoved ? 0 : freqdata_->get(freqx,freqy) );
	    }
	    if ( indexsz_ < mNINT32(data_->get(datapt.x,datapt.y)) )
		indexsz_ = mNINT32(data_->get(datapt.x,datapt.y));
	}
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

void setCellXSize( float sz )		{ cellxsize_ = sz; }

void setCellYSize( float sz )		{ cellysize_ = sz; }

void setCellSize( float sz)
{
    cellxsize_ = cellysize_ = sz;
}

int indexSize() const			{ return indexsz_; }

const TypeSet<RowCol>& selRCs() const	{ return selrowcols_; }

const Interval<int>& usedXPixRg() const	{ return usedxpixrg_; }

void setAreaType(int areatype) 		{ areatype_ = areatype; }

int areaType() const			{ return areatype_; }

void getFreqData( Array2D<float>& freqdata ) const
{
    mDynamicCastGet(Array2DImpl<float>*,freqdataimpl,&freqdata)
    if ( !freqdataimpl ) return;
    freqdataimpl->setSize( freqdata_->info().getSize(0),
	    	      	   freqdata_->info().getSize(1) );
    for ( int idx=0; idx<freqdata_->info().getSize(0); idx++ )
    {
	for ( int idy=0; idy<freqdata_->info().getSize(1); idy++ )
	    freqdata.set( idx, idy, freqdata_->get(idx,idy) );
    }
}

protected:
    uiDataPointSet&			uidps_;
    DataPointSet&			dps_;
    MathExpression*			mathobj_;
    const ObjectSet<SelectionGrp>&	selgrpset_;
    uiWorld2Ui				w2ui_;
    TypeSet<RowCol>			selrowcols_;
    TypeSet<uiDataPointSet::DColID>	modcolidxs_;
    Array2D<float>*			data_;
    Array2D<float>*			freqdata_;
    uiDataPointSetCrossPlotter::AxisData& x_;
    uiDataPointSetCrossPlotter::AxisData& y_;
    Interval<int>			xpixrg_;
    Interval<int>			usedxpixrg_;
    Interval<int>			ypixrg_;
    Threads::Mutex			mutex_;
    bool				changedps_;
    bool				removesel_;
    int					curgrp_;
    int					indexsz_;
    int					nrdone_;
    float				cellxsize_;
    float				cellysize_;
    int					areatype_;
};


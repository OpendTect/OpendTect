#ifndef densitycalc_h
#define densitycalc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Aug 2009
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"

#include "arraynd.h"
#include "mathexpression.h"
#include "polygon.h"
#include "rowcol.h"
#include "task.h"
#include "threadlock.h"

#include "uidatapointset.h"
#include "uiworld2ui.h"


//!\brief calculates densities for teh density display of a crossplot

mClass(uiIo) DensityCalc : public ParallelTask
{ mODTextTranslationClass(DensityCalc);
public:
			DensityCalc(uiDataPointSet&,Array2D<float>* data,
				    uiDataPointSetCrossPlotter::AxisData& x,
				    uiDataPointSetCrossPlotter::AxisData& y,
				    const ObjectSet<SelectionGrp>&,
				    const char* header);

    od_int64		nrDone() const;
    uiString		uiNrDoneText() const;
    od_int64		nrIterations() const;
    bool		doWork(od_int64 start,od_int64 stop,int);

    bool		isSelectionValid(uiDataPointSet::DRowID);
    void		setNrBins(int nrbinx,int nrbiny);
    void		setWorld2Ui(const uiWorld2Ui&);
    void		setMathObj(Math::Expression*);
    void		setModifiedColIds(
				const TypeSet<uiDataPointSet::DColID>&);

    void		setDPSChangeable(bool yn);
    void		setRemSelected(bool yn);
    void		setCurGroup(int curgrp);

    void		setCellXSize(float sz);
    void		setCellYSize(float sz);
    void		setCellSize(float sz);
    int			indexSize() const;

    void		setAreaType(int areatype);
    int			areaType() const;

    void		getFreqData(Array2D<float>&) const;

    const TypeSet<RowCol>&	selRCs() const;
    const Interval<int>&	usedXPixRg() const;


protected:
    uiDataPointSet&			uidps_;
    DataPointSet&			dps_;
    Math::Expression*			mathobj_;
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
    Threads::Lock			lock_;
    bool				changedps_;
    bool				removesel_;
    int					curgrp_;
    int					indexsz_;
    int					nrdone_;
    float				cellxsize_;
    float				cellysize_;
    int					areatype_;
};


DensityCalc::DensityCalc( uiDataPointSet& uidps, Array2D<float>* data,
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

inline od_int64 DensityCalc::nrDone() const
{ return nrdone_; }

inline uiString DensityCalc::uiNrDoneText() const
{ return tr("Points done");}

inline od_int64 DensityCalc::nrIterations() const
{ return dps_.size(); }

inline void DensityCalc::setWorld2Ui( const uiWorld2Ui& w2ui )
{ w2ui_ = w2ui; }

inline void DensityCalc::setMathObj( Math::Expression* mobj )
{ mathobj_ = mobj; }

inline void DensityCalc::setModifiedColIds(
	const TypeSet<uiDataPointSet::DColID>& ids )
{ modcolidxs_ = ids; }

inline void DensityCalc::setDPSChangeable( bool yn )
{ changedps_ = yn; }

inline void DensityCalc::setRemSelected( bool yn )
{ removesel_ = yn; }

inline void DensityCalc::setCurGroup( int curgrp )
{ curgrp_ = curgrp; }

inline void DensityCalc::setCellXSize( float sz )
{ cellxsize_ = sz; }

inline void DensityCalc::setCellYSize( float sz )
{ cellysize_ = sz; }

inline void DensityCalc::setCellSize( float sz)
{ cellxsize_ = cellysize_ = sz; }

inline int DensityCalc::indexSize() const
{ return indexsz_; }

inline const TypeSet<RowCol>& DensityCalc::selRCs() const
{ return selrowcols_; }

inline const Interval<int>& DensityCalc::usedXPixRg() const
{ return usedxpixrg_; }

inline void DensityCalc::setAreaType(int areatype)
{ areatype_ = areatype; }

inline int DensityCalc::areaType() const
{ return areatype_; }

bool DensityCalc::isSelectionValid( uiDataPointSet::DRowID rid )
{
    if ( modcolidxs_.size() && mathobj_ )
    {
	for ( int idx=0; idx<modcolidxs_.size(); idx++ )
	{
	    const float yval = uidps_.getValue( modcolidxs_[idx], rid, true );
	    mathobj_->setVariableValue( idx, yval );
	}

	const float result = mCast(float,mathobj_->getValue());
	if ( mIsZero(result,mDefEps) || mIsUdf(result) )
	    return false;
    }
    return true;
}


void DensityCalc::setNrBins( int nrbinx, int nrbiny )
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


bool DensityCalc::doWork( od_int64 start, od_int64 stop, int )
{
    DataPointSet::RowID rid = mCast(DataPointSet::RowID,start);
    for ( ; rid<=stop; rid++ )
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
			Threads::Locker lckr( lock_ );
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


void DensityCalc::getFreqData( Array2D<float>& freqdata ) const
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


#endif

#pragma once
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          Aug 2009
________________________________________________________________________

-*/

#include "uidatapointsetcrossplot.h"

#include "arrayndimpl.h"
#include "task.h"
#include "threadlock.h"

#include "uidatapointset.h"
#include "uirgbarray.h"


//!\brief calculates densities for teh density display of a crossplot

mClass(uiIo) DensityCalc : public ParallelTask
{ mODTextTranslationClass(DensityCalc);
public:
			DensityCalc(const uiDataPointSet&,
				const uiDataPointSetCrossPlotter::AxisData& x,
				const uiDataPointSetCrossPlotter::AxisData& y,
				OD::Pair<int,int> nrbins);
			~DensityCalc();

    od_int64		nrDone() const		{ return nrdone_; }
    uiString		uiNrDoneText() const	{ return tr("Points done"); }
    od_int64		nrIterations() const	{ return totalnr_; }
    bool		doWork(od_int64 start,od_int64 stop,int);

    int			maxValue() const	{ return maxval_; }
    const Array2D<int>& getDensityData() const	{ return *densitydata_; }

protected:
    const uiDataPointSet&		uidps_;
    const DataPointSet&			dps_;
    Array2DImpl<int>*			densitydata_	= nullptr;
    int					maxval_		= 1;
    double				binsizex_;
    double				binsizey_;
    const uiDataPointSetCrossPlotter::AxisData& x_;
    const uiDataPointSetCrossPlotter::AxisData& y_;

    od_int64				nrdone_		= 0;
    od_int64				totalnr_;
    Threads::Mutex			mutex_;
};


DensityCalc::DensityCalc( const uiDataPointSet& uidps,
		const uiDataPointSetCrossPlotter::AxisData& x,
		const uiDataPointSetCrossPlotter::AxisData& y,
		OD::Pair<int,int> nrbins )
    : ParallelTask("Calculating point density")
    , uidps_(uidps)
    , dps_(uidps.pointSet())
    , x_(x)
    , y_(y)
{
    totalnr_ = dps_.size();

    densitydata_ = new Array2DImpl<int>( nrbins.first(), nrbins.second() );
    densitydata_->setAll( 0 );

    binsizex_ = x_.axis_->range().width(false) / nrbins.first();
    binsizey_ = y_.axis_->range().width(false) / nrbins.second();
}


DensityCalc::~DensityCalc()
{
    delete densitydata_;
}


bool DensityCalc::doWork( od_int64 start, od_int64 stop, int )
{
    DataPointSet::RowID rid = sCast(DataPointSet::RowID,start);
    for ( ; rid<=stop; rid++ )
    {
	nrdone_++;
	if ( dps_.isInactive(rid) )
	    continue;

	const float xval = uidps_.getValue( x_.colid_, rid, true );
	const float yval = uidps_.getValue( y_.colid_, rid, true );
	if ( mIsUdf(xval) || mIsUdf(yval) )
	    continue;

	const float relx = xval - x_.axis_->range().start;
	const float rely = yval - y_.axis_->range().start;
	const int freqx = int(relx/binsizex_);
	const int freqy = int(rely/binsizey_);
	if ( !densitydata_->info().validPos(freqx,freqy) )
	    continue;

	mutex_.lock();
	ValueSeries<int>* stor = densitydata_->getStorage();
	int* storptr = stor ? stor->arr() : nullptr;
	if ( storptr )
	{
	    const od_int64 offset =
		    densitydata_->info().getOffset( freqx, freqy );
	    storptr[offset]++;
	    maxval_ = mMAX( storptr[offset], maxval_ );
	}
	else
	{
	    const int newval = densitydata_->get(freqx,freqy)+1;
	    densitydata_->set( freqx, freqy, newval );
	    maxval_ = mMAX( newval, maxval_ );
	}

	mutex_.unLock();
    }

    return true;
}


mClass(uiIo) Density2RGBArray : public ParallelTask
{
public:
Density2RGBArray( const Array2D<int>& datain, uiRGBArray& rgbarr,
		  const ColTab::Sequence& seq, ColTab::Mapper& mp )
    : ParallelTask()
    , datain_(datain)
    , rgbarr_(rgbarr)
    , seq_(seq)
    , mapper_(mp)
{
    datainsz_[0] = datain.getSize( 0 );
    datainsz_[1] = datain.getSize( 1 );
    rgbsz_[0] = rgbarr.getSize( true );
    rgbsz_[1] = rgbarr.getSize( false );
    cellsz_[0] = float(rgbsz_[0]) / datainsz_[0];
    cellsz_[1] = float(rgbsz_[1]) / datainsz_[1];
}


void setUndefColor( const OD::Color& col )
{
    undefcolor_ = col;
}


od_int64 nrIterations() const override
{
    return rgbsz_[0] * rgbsz_[1];
}


bool doPrepare( int ) override
{
    rgbarr_.clear( undefcolor_ );
    return true;
}


bool doWork( od_int64 start, od_int64 stop, int ) override
{
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const int row = int( idx / rgbsz_[1] );
	const int col = int( idx % rgbsz_[1] );

	const int binx = int( float(row) / cellsz_[0] );
	const int biny = int( float(col) / cellsz_[1] );

	const int val = datain_.get( binx, biny );
	const float mappedval = mapper_.position( float(val) );
	const OD::Color color = val == 0 ? undefcolor_ : seq_.color(mappedval);
	if ( color.isVisible() )
	    rgbarr_.set( row, rgbsz_[1]-col-1, color );
    }

    return true;
}

    const Array2D<int>&		datain_;
    uiRGBArray&			rgbarr_;
    const ColTab::Sequence&	seq_;
    const ColTab::Mapper&	mapper_;

    int				datainsz_[2];
    int				rgbsz_[2];
    float			cellsz_[2];
    OD::Color			undefcolor_	= OD::Color::White();
};

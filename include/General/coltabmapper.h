#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
________________________________________________________________________

-*/

#include "coltabmappersetup.h"
#include "math2.h"
#include "valseries.h"
#include "varlenarray.h"

#define mUndefColIdx    (nrcolors_)

class DataClipper;
class TaskRunner;

namespace ColTab
{

/*!\brief Maps data values to color sequence positions: [0,1].

  This class is not protected against MT write access. So you have to
  set everything before going MT.

  If setup().nrSegs() > 0, the mapper will return the centers of the
  segments only. For example, if nsegs_ == 3, only positions returned are
  1/6, 3/6 and 5/6.

*/

mExpClass(General) Mapper : public CallBacker
{
public:

				Mapper(); //!< default maps [0,1] to [0,1]
				Mapper(const Mapper&);
				~Mapper();
    Mapper&			operator =(const Mapper&);

    MapperSetup&		setup()		{ return *setup_; }
    const MapperSetup&		setup() const	{ return *setup_; }
    void			useSetup(MapperSetup&);
    void			setSetup(const MapperSetup&);

    float			position(float val) const;
				//!< returns position in ColorTable
    static int			indexForValue(const Mapper*,float val,
						int nrcolors,int udfval);
    inline static int		indexForPosition( float ps, int nc, int udfidx )
				{ return indexForValue( 0, ps, nc, udfidx ); }

    Interval<float>		range() const	{ return setup_->range(); }
    const ValueSeries<float>*	data() const	{ return vs_; }
    od_int64			dataSize() const{ return vssz_; }
    DataClipper&		clipper()	{ return clipper_; }
    const DataClipper&		clipper() const	{ return clipper_; }

    void			setRange(Interval<float>);
    void			setData(const ValueSeries<float>*,od_int64 sz,
					TaskRunner* = 0);
				    //!< If data changes, call update()

    void			update(bool full=true,TaskRunner* =0);
				    //!< If !full, will assume data is unchanged

    static float		getPosition(const Interval<float>&,SeqUseMode,
					    int nrsegs,float val);
				//!< only works if mapping is linear on range
    static float		seqPos4RelPos(SeqUseMode,float relpos);
				//!< only works if mapping is linear on range

protected:

    RefMan<MapperSetup>		setup_;
    DataClipper&		clipper_;
    Interval<float>		range_;

    const ValueSeries<float>*	vs_;
    od_int64			vssz_;

    void			doUpdate(bool,TaskRunner*);

};


/*!\brief Maps input data into color indices. Generates a hostogram of values
  while at it.*/

template <class iT>
mClass(General) DataMapper : public ParallelTask
{ mODTextTranslationClass(DataMapper)
public:

			DataMapper(const ColTab::Mapper& map,
				   od_int64 sz,iT nrcolors,
				   const float* inpdata,
				   iT* mappedvals,int mappedvalspacing=1,
				   iT* mappedundef=0,int mappedundefspacing=1);
			DataMapper(const ColTab::Mapper& map,
				   od_int64 sz,iT nrcolors,
				   const ValueSeries<float>& inpvs,
				   iT* mappedvals, int mappedvalspacing=1,
				   iT* mappedundef=0,int mappedundefspacing=1);

    od_int64			nrIterations() const;
    const TypeSet<unsigned int>& histogram() const	{ return histogram_; }
    const SamplingData<float>&	histogramSampling() const
						{ return histogramsampling_; }

private:

    void			initHistogram();
    bool			doWork(od_int64 start,od_int64 stop,int);
    uiString			nrDoneText() const
				{ return tr("Data values mapped"); }

    const ColTab::Mapper&	mapper_;
    const float*		inpdata_;
    const ValueSeries<float>*	inpdatavs_;
    const iT			nrcolors_;
    const od_int64		totalsz_;
    iT*				mappedvals_;
    const int			mappedvalsspacing_;
    iT*				mappedudfs_;
    const int			mappedudfspacing_;

    TypeSet<unsigned int>	histogram_;
    SamplingData<float>		histogramsampling_;
    Threads::Lock		histogramlock_;

};


template <class iT> inline
DataMapper<iT>::DataMapper( const ColTab::Mapper& map, od_int64 sz, iT nrcolors,
			   const float* inpdata,
			   iT* mappedvals, int mappedvalsspacing,
			   iT* mappedudfs, int mappedudfspacing	)
    : ParallelTask( "Color Table Mapping" )
    , mapper_( map )
    , totalsz_( sz )
    , nrcolors_( nrcolors )
    , inpdata_( inpdata )
    , inpdatavs_( 0 )
    , mappedvals_( mappedvals )
    , mappedudfs_( mappedudfs )
    , mappedvalsspacing_( mappedvalsspacing )
    , mappedudfspacing_( mappedudfspacing )
{
    initHistogram();
}


template <class iT> inline
DataMapper<iT>::DataMapper( const ColTab::Mapper& map,
			    od_int64 sz, iT nrcolors,
			   const ValueSeries<float>& inpdata,
			   iT* mappedvals, int mappedvalsspacing,
			   iT* mappedudfs, int mappedudfspacing	)
    : ParallelTask( "Color table mapping" )
    , mapper_( map )
    , totalsz_( sz )
    , nrcolors_( nrcolors )
    , inpdata_( inpdata.arr() )
    , inpdatavs_( inpdata.arr() ? 0 : &inpdata )
    , mappedvals_( mappedvals )
    , mappedudfs_( mappedudfs )
    , mappedvalsspacing_( mappedvalsspacing )
    , mappedudfspacing_( mappedudfspacing )
{
    initHistogram();
}


template <class iT> inline
void DataMapper<iT>::initHistogram()
{
    histogram_.setSize( 255, 0 );
    histogramsampling_.start = mapper_.setup().range().start;
    histogramsampling_.step = mapper_.setup().range().width() / nrcolors_;
}


template <class iT> inline
od_int64 DataMapper<iT>::nrIterations() const
{
    return totalsz_;
}


template <class iT> inline
bool DataMapper<iT>::doWork( od_int64 start, od_int64 stop, int )
{
    TypeSet<float> subhistogram( 255, 0 );
    iT* ptrcurmappedval = mappedvals_ + start*mappedvalsspacing_;
    iT* ptrcurmappedudf = mappedudfs_ ? mappedudfs_ + start*mappedudfspacing_
				      : 0;
    const float* ptrcurinp = inpdata_ + start;
    const ValueSeries<float>* inpdatavs = inpdatavs_;
    const int mappedvalsspacing = mappedvalsspacing_;
    const int mappedudfspacing = mappedudfspacing_;
    const int nrcolors = nrcolors_;
    const int udfcolidx = nrcolors_;
    const int nrsegs = mapper_.setup().nrSegs();
    const SeqUseMode usemode = mapper_.setup().seqUseMode();
    const Interval<float> maprg( mapper_.setup().range() );
    const float rangestart = maprg.start;
    const float rangewidth = maprg.width( false );
    const bool rangehaswidth = !mIsZero(rangewidth,mDefEps);

    int nrdone = 0;
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const float input = inpdatavs ? inpdatavs->value(idx) : *ptrcurinp++;

	float relpos = 0.0f;
	int colidx = udfcolidx;
	bool isudf = true;

	if ( mFastIsFloatDefined(input) )
	{
	    isudf = false;
	    if ( rangehaswidth )
	    {
		relpos = (input-rangestart) / rangewidth;
		if ( nrsegs > 0 )
		    relpos = (0.5f + ((int) (relpos*nrsegs))) / nrsegs;

		if ( relpos > 1.0f )
		    relpos = 1.0f;
		else if ( relpos < 0.0f )
		    relpos = 0.0f;

		float seqposition = Mapper::seqPos4RelPos( usemode, relpos );
		colidx = Mapper::indexForPosition( seqposition,
						   nrcolors, udfcolidx );
	    }
	}

	*ptrcurmappedval = (iT)colidx;
	ptrcurmappedval += mappedvalsspacing;

	if ( ptrcurmappedudf )
	{
	    *ptrcurmappedudf = (iT)(isudf ? 0 : udfcolidx);
	    ptrcurmappedudf += mappedudfspacing;
	}

	if ( !isudf )
	{
	    const int histidx = Mapper::indexForPosition( relpos, nrcolors, 0 );
	    subhistogram[histidx]++;
	}

	if ( (++nrdone) > 100000 )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;
	    if ( !shouldContinue() )
		return false;
	}
    }

    if ( nrdone )
	addToNrDone( nrdone );

    Threads::Locker lckr( histogramlock_ );
    for ( int idx=0; idx<nrcolors_; idx++ )
	histogram_[idx] += subhistogram[idx];

    return true;
}


} // namespace ColTab

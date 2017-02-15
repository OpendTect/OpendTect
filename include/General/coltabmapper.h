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

#define mUndefColIdx    (nrsteps_)

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
    static int			snappedPosition(const Mapper*,float val,
						int nrsteps,int udfval);
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


/*!\brief Uses unmapped data and gathers the info needed to map it. */

template <class iT>
mClass(General) MapperInfoCollector : public ParallelTask
{ mODTextTranslationClass(MapperInfoCollector)
public:
			MapperInfoCollector(const ColTab::Mapper& map,
				   od_int64 sz,iT nrsteps,
				   const float* unmapped,
				   iT* mappedvals,int mappedvalspacing=1,
				   iT* mappedundef=0,int mappedundefspacing=1);
			/*!<separateundef will set every second value to
			    0 or mUndefColIdx depending on if the value
			    is undef or not. Mapped pointer should thus
			    have space for 2*sz */
			MapperInfoCollector(const ColTab::Mapper& map,
				   od_int64 sz,iT nrsteps,
				   const ValueSeries<float>& unmapped,
				   iT* mappedvals, int mappedvalspacing=1,
				   iT* mappedundef=0,int mappedundefspacing=1);
			/*!<separateundef will set every second value to
			    0 or mUndefColIdx depending on if the value
			    is undef or not. Mapped pointer should thus
			    have space for 2*sz */
			~MapperInfoCollector();
    od_int64		nrIterations() const;
    const unsigned int*	getHistogram() const	{ return histogram_; }

private:
    bool			doWork(od_int64 start,od_int64 stop,int);
    uiString			nrDoneText() const
				{ return tr("Data values mapped"); }

    const ColTab::Mapper&	mapper_;
    od_int64			totalsz_;
    const float*		unmapped_;
    const ValueSeries<float>*	unmappedvs_;
    iT*				mappedvals_;
    int				mappedvalsspacing_;
    iT*				mappedudfs_;
    int				mappedudfspacing_;
    const iT			nrsteps_;
    unsigned int*		histogram_;
    Threads::Lock		lock_;
};


template <class iT> inline
MapperInfoCollector<iT>::MapperInfoCollector( const ColTab::Mapper& map, od_int64 sz, iT nrsteps,
			   const float* unmapped,
			   iT* mappedvals, int mappedvalsspacing,
			   iT* mappedudfs, int mappedudfspacing	)
    : ParallelTask( "Color table mapping" )
    , mapper_( map )
    , totalsz_( sz )
    , nrsteps_( nrsteps )
    , unmapped_( unmapped )
    , unmappedvs_( 0 )
    , mappedvals_( mappedvals )
    , mappedudfs_( mappedudfs )
    , mappedvalsspacing_( mappedvalsspacing )
    , mappedudfspacing_( mappedudfspacing )
    , histogram_( new unsigned int[nrsteps+1] )
{
    OD::memZero( histogram_, (mUndefColIdx+1)*sizeof(unsigned int) );
}


template <class iT> inline
MapperInfoCollector<iT>::MapperInfoCollector( const ColTab::Mapper& map, od_int64 sz, iT nrsteps,
			   const ValueSeries<float>& unmapped,
			   iT* mappedvals, int mappedvalsspacing,
			   iT* mappedudfs, int mappedudfspacing	)
    : ParallelTask( "Color table mapping" )
    , mapper_( map )
    , totalsz_( sz )
    , nrsteps_( nrsteps )
    , unmapped_( unmapped.arr() )
    , unmappedvs_( unmapped.arr() ? 0 : &unmapped )
    , mappedvals_( mappedvals )
    , mappedudfs_( mappedudfs )
    , mappedvalsspacing_( mappedvalsspacing )
    , mappedudfspacing_( mappedudfspacing )
    , histogram_( new unsigned int[nrsteps+1] )
{
    OD::memZero( histogram_, (mUndefColIdx+1)*sizeof(unsigned int) );
}


template <class iT> inline
MapperInfoCollector<iT>::~MapperInfoCollector()
{
    delete [] histogram_;
}


template <class iT> inline
od_int64 MapperInfoCollector<iT>::nrIterations() const
{ return totalsz_; }


template <class iT> inline
bool MapperInfoCollector<iT>::doWork( od_int64 start, od_int64 stop, int )
{
    mAllocVarLenArr( unsigned int, histogram,  mUndefColIdx+1);

    OD::memZero( histogram, (mUndefColIdx+1)*sizeof(unsigned int) );

    iT* valresult = mappedvals_+start*mappedvalsspacing_;
    iT* udfresult = mappedudfs_ ? mappedudfs_+start*mappedudfspacing_ : 0;
    const float* inp = unmapped_+start;

    const ValueSeries<float>* unmappedvs = unmappedvs_;
    const int mappedvalsspacing = mappedvalsspacing_;
    const int mappedudfspacing = mappedudfspacing_;
    const int nrsteps = nrsteps_;
    const int udfcolidx = mUndefColIdx;

    const int nrsegs = mapper_.setup().nrSegs();
    const SeqUseMode usemode = mapper_.setup().seqUseMode();
    const Interval<float> maprg( mapper_.setup().range() );
    const float rangewidth = maprg.width( false );
    const bool rangehaswidth = !mIsZero(rangewidth,mDefEps);
    const float rangestart = maprg.start;

    int nrdone = 0;
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const float input = unmappedvs ? unmappedvs->value(idx) : *inp++;

	float position = 0.0f;
	bool isudf = true;

	if ( mFastIsFloatDefined(input) )
	{
	    isudf = false;
	    if ( rangehaswidth )
	    {
		position = (input-rangestart) / rangewidth;
		if ( nrsegs > 0 )
		    position = (0.5f + ((int) (position*nrsegs))) / nrsegs;

		if ( position > 1.0f )
		    position = 1.0f;
		else if ( position < 0.0f )
		    position = 0.0f;

		position = Mapper::seqPos4RelPos( usemode, position );
		position = Mapper::snappedPosition( 0, position, nrsteps, -1 );
	    }
	}

	const iT res = (iT)( isudf ? udfcolidx : (int)position );

	*valresult = res;
	valresult += mappedvalsspacing;

	if ( udfresult )
	{
	    *udfresult = (iT)(isudf ? 0 : udfcolidx);
	    udfresult += mappedudfspacing;
	}

	histogram[res]++;

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

    Threads::Locker lckr( lock_ );
    for ( int idx=0; idx<=mUndefColIdx; idx++ )
	histogram_[idx] += histogram[idx];

    return true;
}


} // namespace ColTab

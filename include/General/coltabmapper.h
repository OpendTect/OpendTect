#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "coltab.h"
#include "math2.h"
#include "valseries.h"
#include "threadlock.h"
#include "varlenarray.h"

template <class T> class ArrayND;

#define mUndefColIdx    (nrsteps_)

class DataClipper;
class TaskRunner;

namespace ColTab
{

/*!
\brief Setup class for colortable Mapper.
*/

mExpClass(General) MapperSetup : public CallBacker
{
public:
			MapperSetup();
    enum Type		{ Fixed, Auto, HistEq };
			mDeclareEnumUtils(Type);

    mDefSetupClssMemb(MapperSetup,Type,type)
    mDefSetupClssMemb(MapperSetup,Interval<float>,cliprate)	//!< Auto
    mDefSetupClssMemb(MapperSetup,bool,autosym0)	//!< Auto and HistEq.
    mDefSetupClssMemb(MapperSetup,float,symmidval)	//!< Auto and HistEq.
							//!< Usually mUdf(float)
    mDefSetupClssMemb(MapperSetup,int,maxpts)		//!< Auto and HistEq
    mDefSetupClssMemb(MapperSetup,int,nrsegs)		//!< All
    mDefSetupClssMemb(MapperSetup,bool,flipseq)		//!< All
    mDefSetupClssMemb(MapperSetup,Interval<float>,range)

    bool			operator==(const MapperSetup&) const;
    bool			operator!=(const MapperSetup&) const;
    MapperSetup&		operator=(const MapperSetup&);

    bool			needsReClip(const MapperSetup&) const;
				//!<Is new clip necessary if set to this

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    static const char*		sKeyClipRate()	{ return "Clip Rate"; }
    static const char*		sKeyAutoSym()	{ return "Auto Sym"; }
    static const char*		sKeySymMidVal()	{ return "Sym Mid Value"; }
    static const char*		sKeyStarWidth()	{ return "Start_Width"; }
    static const char*		sKeyRange()	{ return "Range"; }
    static const char*		sKeyFlipSeq()	{ return "Flip seq"; }

    void			triggerRangeChange();
    void			triggerAutoscaleChange();
    mutable Notifier<MapperSetup>	rangeChange;
    mutable Notifier<MapperSetup>	autoscaleChange;

    void			setAutoScale(bool);
};


/*!
\brief Maps data values to colortable positions: [0,1].

  If nrsegs_ > 0, the mapper will return the centers of the segments only. For
  example, if nsegs_ == 3, only positions returned are 1/6, 3/6 and 5/6.
*/

mExpClass(General) Mapper
{
public:

				Mapper(); //!< defaults maps from [0,1] to [0,1]
				Mapper(const Mapper&);
				Mapper(const Mapper&,bool shareclipper);
				~Mapper();

    Mapper&			operator =(const Mapper&);

    float			position(float val) const;
				//!< returns position in ColorTable
    static int			snappedPosition(const Mapper*,float val,
						int nrsteps,int udfval);
    const Interval<float>&	range() const;
    bool			isFlipped() const    { return setup_.flipseq_; }
    const ValueSeries<float>*	data() const	     { return vs_; }
    od_int64			dataSize() const     { return datasz_; }

    void			setFlipped( bool yn ) { setup_.flipseq_ = yn; }

    void			setRange(const Interval<float>&);
    void			setData(const float*,od_int64 sz,
					TaskRunner* =nullptr);
    void			setData(const ValueSeries<float>&,
					TaskRunner* =nullptr);
    void			setData(const ArrayND<float>&,
					TaskRunner* =nullptr);
				//!< If data changes, call update()

    void			update(bool full=true,TaskRunner* =nullptr);
				//!< If !full, will assume data is unchanged
    MapperSetup			setup_;

protected:

    DataClipper*		clipper_;
    bool			clipperismine_;

    const ArrayND<float>*	arrnd_		= nullptr;
    const ValueSeries<float>*	vs_		= nullptr;
    const float*		dataptr_	= nullptr;
    od_int64			datasz_		= -1;

};


/*!
\brief Takes a Mapper, unmapped data and maps it.
*/

template <class T>
mClass(General) MapperTask : public ParallelTask
{ mODTextTranslationClass(MapperTask)
public:
			MapperTask(const ColTab::Mapper& map,
				   od_int64 sz,T nrsteps,
				   const float* unmapped,
				   T* mappedvals,int mappedvalspacing=1,
				   T* mappedundef=0,int mappedundefspacing=1);
			/*!<separateundef will set every second value to
			    0 or mUndefColIdx depending on if the value
			    is undef or not. Mapped pointer should thus
			    have space for 2*sz */
			MapperTask(const ColTab::Mapper& map,
				   od_int64 sz,T nrsteps,
				   const ValueSeries<float>& unmapped,
				   T* mappedvals, int mappedvalspacing=1,
				   T* mappedundef=0,int mappedundefspacing=1);
			/*!<separateundef will set every second value to
			    0 or mUndefColIdx depending on if the value
			    is undef or not. Mapped pointer should thus
			    have space for 2*sz */
			~MapperTask();
    od_int64		nrIterations() const;
    const unsigned int*	getHistogram() const	{ return histogram_; }

private:
    bool			doWork(od_int64 start,od_int64 stop,int);
    uiString			uiNrDoneText() const
				{ return tr("Data values mapped"); }

    const ColTab::Mapper&	mapper_;
    od_int64			totalsz_;
    const float*		unmapped_;
    const ValueSeries<float>*	unmappedvs_;
    T*				mappedvals_;
    int				mappedvalsspacing_;
    T*				mappedudfs_;
    int				mappedudfspacing_;
    T				nrsteps_;
    unsigned int*		histogram_;
    Threads::Lock		lock_;
};


template <class T> inline
MapperTask<T>::MapperTask( const ColTab::Mapper& map, od_int64 sz, T nrsteps,
			   const float* unmapped,
			   T* mappedvals, int mappedvalsspacing,
			   T* mappedudfs, int mappedudfspacing	)
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


template <class T> inline
MapperTask<T>::MapperTask( const ColTab::Mapper& map, od_int64 sz, T nrsteps,
			   const ValueSeries<float>& unmapped,
			   T* mappedvals, int mappedvalsspacing,
			   T* mappedudfs, int mappedudfspacing	)
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


template <class T> inline
MapperTask<T>::~MapperTask()
{
    delete [] histogram_;
}


template <class T> inline
od_int64 MapperTask<T>::nrIterations() const
{ return totalsz_; }


template <class T> inline
bool MapperTask<T>::doWork( od_int64 start, od_int64 stop, int )
{
    mAllocVarLenArr( unsigned int, histogram,  mUndefColIdx+1);

    OD::memZero( histogram, (mUndefColIdx+1)*sizeof(unsigned int) );

    T* valresult = mappedvals_+start*mappedvalsspacing_;
    T* udfresult = mappedudfs_ ? mappedudfs_+start*mappedudfspacing_ : 0;
    const float* inp = unmapped_+start;

    const ValueSeries<float>* unmappedvs = unmappedvs_;
    const int mappedvalsspacing = mappedvalsspacing_;
    const int mappedudfspacing = mappedudfspacing_;
    const int nrsteps = nrsteps_;
    const int udfcolidx = mUndefColIdx;

    const int nrsegs = mapper_.setup_.nrsegs_;
    const bool flipseq = mapper_.setup_.flipseq_;
    const float rangewidth = mapper_.setup_.range_.width( false );
    const bool rangehaswidth = !mIsZero(rangewidth,mDefEps);
    const float rangestart = mapper_.setup_.range_.start;

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

		if ( flipseq )
		    position = 1.0f - position;

		position *= nrsteps;

		if ( position > nrsteps-0.9f )
		    position = nrsteps - 0.9f;
		else if ( position < 0.0f )
		    position = 0.0f;
	    }
	}

	const T res = (T) ( isudf ? udfcolidx : (int) position );

	*valresult = res;
	valresult += mappedvalsspacing;

	if ( udfresult )
	{
	    *udfresult = (T) (isudf ? 0 : udfcolidx);
	    udfresult += mappedudfspacing;
	}

	histogram[res]++;

	if ( nrdone > 100000 )
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


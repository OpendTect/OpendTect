#ifndef coltabmapper_h
#define coltabmapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabmapper.h,v 1.32 2012-08-30 11:16:48 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "coltab.h"
#include "thread.h"
#include "valseries.h"
#include "varlenarray.h"

#define mUndefColIdx    (nrsteps_)

class DataClipper;
class IOPar;
class TaskRunner;

namespace ColTab
{

/*!\brief Maps data values to color table positions: [0,1]

  If nrsegs_ > 0, the mapper will return the centers of the segments only. For
  example, if nsegs_ == 3, only positions returned are 1/6, 3/6 and 5/6.
 
 */
mClass(General) MapperSetup : public CallBacker
{
public:
			MapperSetup();
    enum Type		{ Fixed, Auto, HistEq };
    			DeclareEnumUtils(Type);

    mDefSetupClssMemb(MapperSetup,Type,type);
    mDefSetupClssMemb(MapperSetup,Interval<float>,cliprate);	//!< Auto
    mDefSetupClssMemb(MapperSetup,bool,autosym0);	//!< Auto and HistEq.
    mDefSetupClssMemb(MapperSetup,float,symmidval);	//!< Auto and HistEq.
    							//!< Usually mUdf(float)
    mDefSetupClssMemb(MapperSetup,int,maxpts);		//!< Auto and HistEq
    mDefSetupClssMemb(MapperSetup,int,nrsegs);		//!< All
    mDefSetupClssMemb(MapperSetup,bool,flipseq);	//!< All
    mDefSetupClssMemb(MapperSetup,Interval<float>,range);

    bool 			operator==(const MapperSetup&) const;
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
};


mClass(General) Mapper
{
public:

				Mapper(); //!< defaults maps from [0,1] to [0,1]
				~Mapper();

    float			position(float val) const;
    				//!< returns position in ColorTable
    static int			snappedPosition(const Mapper*,float val,
	    					int nrsteps,
	    				int udfval);
    const Interval<float>& range() const;
    bool		isFlipped() const { return setup_.flipseq_; }
    const ValueSeries<float>* data() const
			{ return vs_; }
    od_int64		dataSize() const
			{ return vssz_; }

    void		setFlipped(bool yn) { setup_.flipseq_ = yn; }

    void		setRange( const Interval<float>& rg );
    void		setData(const ValueSeries<float>*,od_int64 sz,
	    			TaskRunner* = 0);
    			//!< If data changes, call update()

    void		update(bool full=true, TaskRunner* = 0);
    			//!< If !full, will assume data is unchanged
			//
    MapperSetup		setup_;

protected:

    DataClipper&		clipper_;

    const ValueSeries<float>*	vs_;
    od_int64			vssz_;

};


/*!Takes a Mapper, unmapped data and maps it.*/
template <class T>
class MapperTask : public ParallelTask
{
public:    
    			MapperTask(const ColTab::Mapper& map,
				   od_int64 sz,int nrsteps,
				   const float* unmapped,
				   T* mappedvals,int mappedvalspacing=1,
				   T* mappedundef=0,int mappedundefspacing=1);
			/*!<separateundef will set every second value to
			    0 or mUndefColIdx depending on if the value
			    is undef or not. Mapped pointer should thus
			    have space for 2*sz */
    			MapperTask(const ColTab::Mapper& map,
				   od_int64 sz,int nrsteps,
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

    Threads::Mutex		lock_;
    const ColTab::Mapper&	mapper_;
    od_int64			totalsz_;
    const float*		unmapped_;
    const ValueSeries<float>*	unmappedvs_;
    T*				mappedvals_;
    int				mappedvalsspacing_;
    T*				mappedudfs_;
    int				mappedudfspacing_;
    int				nrsteps_;
    unsigned int*		histogram_;
};


template <class T> inline
MapperTask<T>::MapperTask( const ColTab::Mapper& map, od_int64 sz, int nrsteps, 
			   const float* unmapped,
			   T* mappedvals, int mappedvalsspacing,
       			   T* mappedudfs, int mappedudfspacing	)
    : mapper_( map )
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
    memset( histogram_, 0, (mUndefColIdx+1)*sizeof(unsigned int) );
}


template <class T> inline
MapperTask<T>::MapperTask( const ColTab::Mapper& map, od_int64 sz, int nrsteps, 
			   const ValueSeries<float>& unmapped,
			   T* mappedvals, int mappedvalsspacing,
       			   T* mappedudfs, int mappedudfspacing	)
    : mapper_( map )
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
    memset( histogram_, 0, (mUndefColIdx+1)*sizeof(unsigned int) );
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
   
    memset( histogram, 0, (mUndefColIdx+1)*sizeof(unsigned int) );

    T* valresult = mappedvals_+start*mappedvalsspacing_;
    T* udfresult = mappedudfs_ ? mappedudfs_+start*mappedudfspacing_ : 0;
    const float* inp = unmapped_+start;

    int nrdone = 0;
    for ( int idx=start; idx<=stop; idx++, nrdone++ )
    {
	const float input = unmappedvs_ ? unmappedvs_->value(idx) : *inp;
	T res;
	const bool isudf = mIsUdf(input);
	if ( isudf )
	    res = mUndefColIdx;
	else
	    res = ColTab::Mapper::snappedPosition( &mapper_,input,
						    nrsteps_, mUndefColIdx );

	*valresult = res;
	if ( udfresult )
	{
	    *udfresult = isudf ? 0 : mUndefColIdx;
	    udfresult += mappedudfspacing_;
	}

	histogram[res]++;
	valresult+= mappedudfspacing_;
	inp++;

	if ( nrdone>10000 )
	{
	    addToNrDone( nrdone );
	    nrdone = 0;
	    if ( !shouldContinue() )
		return false;
	}
    }

    if ( nrdone )
    {
	addToNrDone( nrdone );
	nrdone = 0;
    }

    lock_.lock();
    for ( int idx=0; idx<=mUndefColIdx; idx++ )
	histogram_[idx] += histogram[idx];

    lock_.unLock();
    
    return true;
}


} // namespace ColTab

#endif


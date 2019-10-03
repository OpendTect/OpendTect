#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2007
________________________________________________________________________

*/


#include "seiscommon.h"
#include "arraynd.h"
#include "bin2d.h"
#include "objectset.h"
#include "uistring.h"

template <class T> class Array2D;
class SeisTrc;
class SeisTrcBuf;


namespace Seis
{

class SelData;
class Provider;


/*!\brief Reads seismic data into buffers providing a Moving Virtual Subcube
          of seismic data.

Note that the provider may skip incomplete parts.

The get() method returns a pointer to the trace, where you specify the
inline and crossline number relative to the center. This is irrespective
the steps in the cube's numbers. Therefore, the actual inline number of
get(1,0) may be 10 higher than get(0,0) .

The advance() method moves the reader one step further along the seismic
storage. The return value will tell you whether there is a new position
available to work on, or that more traces need to be read first.

You can specify two stepouts: required and desired. The required stepout
traces will always be available when the return of advance() is DataOK.
If "Buffering" is returned, then the MSCProvider is still gathering more
traces.

 */


mExpClass(Seis) MSCProvider
{ mODTextTranslationClass(Seis::MSCProvider);
public:

    mUseType( Pos,	GeomID );
    mUseType( Pos,	IdxPair );
    mUseType( IdxPair,	pos_type );
    typedef float	z_type;
    typedef Interval<z_type> z_rg_type;

			MSCProvider(const DBKey&);
			MSCProvider(Provider&);
    bool		isOK() const		{ return uirv_.isOK(); }
    virtual		~MSCProvider();

    bool		is2D() const;
    BufferString	name() const; // cube name

			// use the following after prepareWork
			// but before the first next()
    void		forceFloatData( bool yn )
			{ intofloats_ = yn; }
    void		setStepout(pos_type,bool required);
    void		setStepout(pos_type,pos_type,bool required);
    void		setStepout(Array2D<bool>* mask);
			/*!< mask has 2m+1 * 2n+1 entries and becomes mine. */
    void		setStepoutStep( pos_type trcnr )
			{ stepoutstep_.row() = 0; stepoutstep_.col() = trcnr; }
    void		setStepoutStep( pos_type i, pos_type c )
			{ stepoutstep_.row() = i; stepoutstep_.col() = c; }
    pos_type		inlStepout( bool req ) const
			{ return req ? reqstepout_.row() : desstepout_.row(); }
    pos_type		crlStepout( bool req ) const
			{ return req ? reqstepout_.col() : desstepout_.col(); }
    pos_type		trcStepout( bool req ) const
			{ return req ? reqstepout_.col() : desstepout_.col(); }
    void		setSelData(SelData*);
    void		setSelData(const SelData&);
    void		setZExtension(const z_rg_type&);

    enum AdvanceState	{ NewPosition, Buffering, EndReached, Error };
    AdvanceState	advance();
    const uiRetVal&	errMsg() const		{ return uirv_; }
    bool		toNextPos(); //!< end==false but no errMsg

    IdxPair		curPos() const;
    BinID		curBinID() const;
    Bin2D		curBin2D() const;
    GeomID		curGeomID() const;
    pos_type		curTrcNr() const;
    SeisTrc*		curTrc();
    SeisTrc*		getAt(pos_type deltatrcnr);
    SeisTrc*		getAt(pos_type deltainl,pos_type deltacrl);
    SeisTrc*		getFor(const Bin2D&);
    SeisTrc*		getFor(const BinID&);
    const SeisTrc*	curTrc() const
			{ return mSelf().curTrc(); }
    const SeisTrc*	getAt( pos_type dtrc ) const
			{ return mSelf().getAt( dtrc ); }
    const SeisTrc*	getAt( pos_type di, pos_type dc ) const
			{ return mSelf().getAt( di, dc ); }
    const SeisTrc*	getFor( const Bin2D& b2d ) const
			{ return mSelf().getFor( b2d ); }
    const SeisTrc*	getFor( const BinID& bid ) const
			{ return mSelf().getFor( bid ); }

    int			comparePos(const MSCProvider&) const;
			//!< 0 = equal; -1 = I need next(), 1 = other needs next
    od_int64		estimatedNrTraces() const; //!< returns -1 when unknown

    inline GeomID	geomID() const		{ return curGeomID(); }
    Provider&		provider()		{ return *prov_; }
    const Provider&	provider() const	{ return *prov_; }

protected:

    uiRetVal		uirv_; // keep before prov_
    Provider*		prov_			= nullptr;
    const bool		provmine_;
    ObjectSet<SeisTrcBuf> tbufs_;
    IdxPair		reqstepout_		= IdxPair(0,0);
    IdxPair		desstepout_		= IdxPair(0,0);
    IdxPair		stepoutstep_;
    Array2D<bool>*	reqmask_		= nullptr;
    bool		intofloats_		= false;
    bool		workstarted_		= false;
    bool		atend_			= false;

    GeomID		curgeomid_;

			// Indexes of new pos ready, -1 while buffering.
    int			bufidx_			= -1;
    int			trcidx_			= -1;
			// Indexes of next position to be examined.
    int			pivotidx_;
    int			pivotidy_;

    bool		startWork();
    bool		readTrace(SeisTrc&);
    bool		isReqBoxFilled() const;
    bool		doAdvance();

};


mGlobal(Seis) bool advance(ObjectSet<MSCProvider>&,uiRetVal&);


} // namespace Seis

mDeprecated typedef Seis::MSCProvider SeisMSCProvider;

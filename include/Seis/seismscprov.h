#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2007
________________________________________________________________________

*/


#include "seiscommon.h"
#include "arraynd.h"
#include "rowcol.h"
#include "objectset.h"
#include "uistring.h"

template <class T> class Array2D;
class IOObj;
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
If "Buffering" is returned, then the provider is still gathering more
traces.

 */


mExpClass(Seis) MSCProvider
{ mODTextTranslationClass(Seis::MSCProvider);
public:

			MSCProvider(const DBKey&);
				//!< Use any real user entry from '.omf' file
    virtual		~MSCProvider();

    bool		is2D() const;
    BufferString	name() const; // cube name

			// use the following after prepareWork
			// but before the first next()
    void		forceFloatData( bool yn )
			{ intofloats_ = yn; }
    void		setStepout(int,int,bool required);
    void		setStepout(Array2D<bool>* mask);
			/*!< mask has 2m+1 * 2n+1 entries and becomes mine. */
    void		setStepoutStep( int i, int c )
			{ stepoutstep_.row() = i; stepoutstep_.col() = c; }
    int			inlStepout( bool req ) const
			{ return req ? reqstepout_.row() : desstepout_.row(); }
    int			crlStepout( bool req ) const
			{ return req ? reqstepout_.col() : desstepout_.col(); }
    void		setSelData(Seis::SelData*);
			//!< seldata becomes mine

    enum AdvanceState	{ NewPosition, Buffering, EndReached, Error };
    AdvanceState	advance();
    uiString		errMsg() const		{ return uirv_; }

    BinID		getPos() const;
    int			getTrcNr() const;
    SeisTrc*		get(int deltainl,int deltacrl);
    SeisTrc*		get(const BinID&);
    const SeisTrc*	get( int i, int c ) const
			{ return const_cast<MSCProvider*>(this)->get(i,c); }
    const SeisTrc*	get( const BinID& bid ) const
			{ return const_cast<MSCProvider*>(this)->get(bid); }

    int			comparePos(const MSCProvider&) const;
			//!< 0 = equal; -1 means I need to next(), 1 the other
    int			estimatedNrTraces() const; //!< returns -1 when unknown

    Provider*		provider()		{ return prov_; }
    const Provider*	provider() const	{ return prov_; }

protected:

    Provider*		prov_;
    ObjectSet<SeisTrcBuf> tbufs_;
    RowCol		reqstepout_;
    RowCol		desstepout_;
    RowCol		stepoutstep_;
    Array2D<bool>*	reqmask_;
    bool		intofloats_;
    bool		workstarted_;
    bool		atend_;

    uiRetVal		uirv_;
    int			curlinenr_;
    mutable int		estnrtrcs_;

			// Indexes of new pos ready, equals -1 while buffering.
    int			bufidx_;
    int			trcidx_;
			// Indexes of next position to be examined.
    int			pivotidx_;
    int			pivotidy_;

    bool		startWork();
    bool		readTrace(SeisTrc&);
    bool		isReqBoxFilled() const;
    bool		doAdvance();

};

} // namespace Seis

mDeprecated typedef Seis::MSCProvider SeisMSCProvider;

#ifndef seismscprov_h
#define seismscprov_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jan 2007
 RCS:		$Id: seiscubeprov.h,v 1.4 2007-04-06 15:04:59 cvsjaap Exp $
________________________________________________________________________

*/


#include "position.h"
#include "sets.h"
class IOObj;
class MultiID;
class SeisTrc;
class SeisTrcBuf;
class SeisSelData;


/*!\brief Reads seismic data into buffers providing a Moving Virtual Subcube
          of seismic data.

This is a SeisTrcGroup that allows advancing by reading traces from storage.
Note that the provider may skip incomplete parts.

The get() method returns a pointer to the trace, where you specify the
inline and crossline number relative to the center. This is irrespective
the steps in the cube's numbers. Therefore, the actual inline number of
get(1,0) may be 10 higher than get(0,0) .

The next() method moves the cube one further along the seismic storage.
Depending on what is available in the cube, the return value will tell
you whether you can work on the new position.

You canspecify two stepouts: required and desired. The required stepout traces
will always be available when the return of next() is DataOK. If DataIncomplete
is returned, then the provider is still gathering more traces.
 
 */


class SeisMSCProvider
{
public:

			SeisMSCProvider(const MultiID&);
				//!< Use any real user entry from '.omf' file
			SeisMSCProvider(const IOObj&);
				//!< Use any real user entry from '.omf' file
			SeisMSCProvider(const char* fnm,bool is2d);
				//!< Use 'loose' CBVS files only.
    virtual		~SeisMSCProvider();

    bool		prepareWork();
    bool		is2D() const;

    			// use the following after prepareWork
    			// but before the first next()
    void		forceFloatData( bool yn )
    			{ intofloats_ = yn; }
    void		setStepoutStep( int i, int c )
			{ stepoutstep_.inl = i; stepoutstep_.crl = c; }
    void		setStepout(int,int,bool required);
    int			inlStepout( bool req ) const
    			{ return req ? reqstepout_.inl : desstepout_.inl; }
    int			crlStepout( bool req ) const
    			{ return req ? reqstepout_.crl : desstepout_.crl; }

    enum State		{ DataOK, DataIncomplete, NoMoreData, Error }
    State		advance();	
    State		state()			{ return state_; }
    const char*		errMsg() const		{ return errmsg_; }

    const BinID&	getPos() const;
    int			getTrcNr() const;
    SeisTrc*		get(int inl,int crl);	//!< Relative position. Fast.
    SeisTrc*		get(const BinID&);	//!< Absolute position. Slower.
    const SeisTrc*	get( int i, int c ) const
			{ return const_cast<SeisMSCProvider*>(this)->get(i,c); }
    const SeisTrc*	get( const BinID& bid ) const
			{ return const_cast<SeisMSCProvider*>(this)->get(bid); }

    int			comparePos(const SeisMSCProvider&) const;
    			//!< 0 = equal; -1 means I need to next(), 1 the other
    int			estimatedNrTraces() const; //!< returns -1 when unknown

    SeisTrcReader&	reader()		{ return rdr_; }
    const SeisTrcReader& reader() const		{ return rdr_; }

protected:

    SeisTrcReader&	rdr_;
    ObjectSet<SeisTrcBuf> tbufs_;
    RowCol		reqstepout_;
    RowCol		desstepout_;
    BinID		stepoutstep_;
    bool		intofloats_;
    SeisSelData*	seldata_;
    bool		workstarted_;

    RowCol		curpos_;
    BufferString	errmsg_;
    mutable int		estnrtrcs_;
    int			reqmininl_;
    int			reqmaxinl_;
    int			reqmincrl_;
    int			reqmaxcrl_;

    int			newposidx_;	// Index of new position ready, 
					// equals -1 while buffering.
    int			pivotidx_;	// Next position to be examined.
    
    void		init();
    void		doUsePar(const IOPar&);
    void		getIdxs(int,int,int&,int&);
    void		getIdxs(const BinID&,int&,int&);
    int			selRv(const SeisSelData*,bool) const;

    bool		startWork();
    bool		isSingleTrc() const
			{ return desstepout_.r() == 0 && desstepout_.c() == 0; }

    bool 		gapInReqBox(int pivotidx,bool upwards) const;
};


#endif

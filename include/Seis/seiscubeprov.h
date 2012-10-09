#ifndef seismscprov_h
#define seismscprov_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jan 2007
 RCS:		$Id$
________________________________________________________________________

*/


#include "arraynd.h"
#include "cubesampling.h"
#include "rowcol.h"
#include "objectset.h"

template <class T> class Array2D;
class BinID;
class IOObj;
class MultiID;
class LineKey;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcReader;
namespace Seis		{ class SelData; }
class TaskRunner;


/*!\brief Reads seismic data into buffers providing a Moving Virtual Subcube
          of seismic data.

This is a SeisTrcGroup that allows advancing by reading traces from storage.
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


mClass SeisMSCProvider
{
public:

			SeisMSCProvider(const MultiID&);
				//!< Use any real user entry from '.omf' file
			SeisMSCProvider(const IOObj&);
				//!< Use any real user entry from '.omf' file
			SeisMSCProvider(const char* fnm);
				//!< 'loose' 3D Post-stack CBVS files only.
    virtual		~SeisMSCProvider();

    bool		is2D() const;
    bool		prepareWork();
    			//!< Opens the input data. Can still set stepouts etc.

    			// use the following after prepareWork
    			// but before the first next()
    void		forceFloatData( bool yn )
    			{ intofloats_ = yn; }
    void		setStepout(int,int,bool required);
    void		setStepout(Array2D<bool>* mask);
			/*!< mask has 2m+1 * 2n+1 entries and becomes mine. */
    void		setStepoutStep( int i, int c )
			{ stepoutstep_.row = i; stepoutstep_.col = c; }
    int			inlStepout( bool req ) const
    			{ return req ? reqstepout_.row : desstepout_.row; }
    int			crlStepout( bool req ) const
    			{ return req ? reqstepout_.col : desstepout_.col; }
    void		setSelData(Seis::SelData*);
    			//!< seldata becomes mine

    enum AdvanceState	{ NewPosition, Buffering, EndReached, Error };
    AdvanceState	advance();	
    const char*		errMsg() const		{ return errmsg_.str(); }

    BinID		getPos() const;
    int			getTrcNr() const;
    SeisTrc*		get(int deltainl,int deltacrl);
    SeisTrc*		get(const BinID&);
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
    RowCol		stepoutstep_;
    Array2D<bool>*	reqmask_;
    bool		intofloats_;
    bool		workstarted_;
    enum ReadState	{ NeedStart, ReadOK, ReadAtEnd, ReadErr };
    ReadState		readstate_;

    BufferString	errmsg_;
    mutable int		estnrtrcs_;

    			// Indexes of new pos ready, equals -1 while buffering.
    int			bufidx_; 
    int			trcidx_;
    			// Indexes of next position to be examined.
    int			pivotidx_;
    int			pivotidy_;	
    
    void		init();
    bool		startWork();
    int			readTrace(SeisTrc&);
    bool 		isReqBoxFilled() const;
    bool 		doAdvance();
};


mClass SeisFixedCubeProvider
{
public:
    			SeisFixedCubeProvider(const MultiID&);
			~SeisFixedCubeProvider();

    void		clear();
    bool		isEmpty() const;
    bool		readData(const CubeSampling&,TaskRunner* tr=0);
    bool		readData(const CubeSampling&,const LineKey* lk,
	    			 TaskRunner* tr=0);

    const SeisTrc*	getTrace(const BinID&) const;
    const char*		errMsg() const;

protected:

    Array2D<SeisTrc*>*	data_;

    CubeSampling	cs_;
    IOObj*		ioobj_;
    BufferString	errmsg_;

};

#endif

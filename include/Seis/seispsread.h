#ifndef seispsread_h
#define seispsread_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "position.h"
class IOPar;
class SeisTrc;
class SeisTrcBuf;
class BufferStringSet;
namespace PosInfo { class CubeData; class Line2DData; }


/*!\brief reads from a pre-stack seismic data store.

 Some data stores like attribute stores have a symbolic name for each sample. In
 that case, getSampleNames may return true.

*/

mClass SeisPSReader
{
public:

    virtual		~SeisPSReader()					{}
    virtual bool	is2D() const					= 0;

    virtual void	usePar(const IOPar&)				{}

    virtual const char*	errMsg() const					= 0;
    virtual SeisTrc*	getTrace(const BinID&,int nr=0) const;
    virtual bool	getGather(const BinID&,SeisTrcBuf&) const	= 0;

    virtual bool	getSampleNames(BufferStringSet&) const
    			{ return false; }

};

/*!\brief reads from a 3D pre-stack seismic data store. */

mClass SeisPS3DReader : public SeisPSReader
{
public:

    bool		is2D() const		{ return false; }

    virtual const PosInfo::CubeData&	posData() const			= 0;

};


/*!\brief reads from a 2D pre-stack seismic data store. */

mClass SeisPS2DReader : public SeisPSReader
{
public:
    			SeisPS2DReader( const char* lnm )
			    : lnm_(lnm)		{}
    bool		is2D() const		{ return true; }
    const char*		lineName() const	{ return lnm_.buf(); }

    			// Cannot use name overloading: seems gcc prob
    SeisTrc*		getTrc( int trcnr, int nr=0 ) const
			{ return getTrace( BinID(0,trcnr), nr ); }
    bool		getGath( int trcnr, SeisTrcBuf& b ) const
			{ return getGather( BinID(0,trcnr), b ); }

    virtual const PosInfo::Line2DData&	posData() const		= 0;

protected:

    BufferString	lnm_;

};


#endif

#ifndef seispsread_h
#define seispsread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsread.h,v 1.5 2008-01-14 12:06:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class BinID;
class IOPar;
class SeisTrcBuf;
class BufferStringSet;
namespace PosInfo { class CubeData; class Line2DData; }


/*!\brief reads from a pre-stack seismic data store.

 Based on a BinID a gather may be available. No writing.

 Some data stores like attribute stores have a symbolic name for each sample. In
 that case, getSampleNames may return true;

*/

class SeisPSReader
{
public:

    virtual		~SeisPSReader()					{}

    virtual void	usePar(const IOPar&)				{}

    virtual bool	getGather(const BinID&,SeisTrcBuf&) const	= 0;
    virtual const char*	errMsg() const					= 0;

    virtual const PosInfo::CubeData&	posData() const			= 0;

    virtual bool	getSampleNames(BufferStringSet&) const
    			{ return false; }

};

/*!\brief reads from a 2D pre-stack seismic data store.

 A gather may be available for a trace number (or Coord). No writing.

*/

class SeisPS2DReader
{
public:

    virtual		~SeisPS2DReader()			{}

    virtual void	usePar(const IOPar&)			{}

    virtual bool	getGather(int,SeisTrcBuf&) const	= 0;
    virtual const char*	errMsg() const				= 0;

    virtual const PosInfo::Line2DData&	posData() const		= 0;

    virtual bool	getSampleNames(BufferStringSet&) const
    			{ return false; }

};


#endif

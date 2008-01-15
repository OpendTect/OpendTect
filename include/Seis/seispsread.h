#ifndef seispsread_h
#define seispsread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsread.h,v 1.6 2008-01-15 16:19:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class BinID;
class IOPar;
class SeisTrcBuf;
class BufferStringSet;
namespace PosInfo { class CubeData; class Line2DData; }


/*!\brief reads from a pre-stack seismic data store.

 No writing.

 Some data stores like attribute stores have a symbolic name for each sample. In
 that case, getSampleNames may return true.

 When the reader is 2D, the crl of the BinID should contain the trace number.

*/

class SeisPSReader
{
public:

    virtual		~SeisPSReader()					{}

    virtual void	usePar(const IOPar&)				{}

    virtual bool	getGather(const BinID&,SeisTrcBuf&) const	= 0;
    virtual const char*	errMsg() const					= 0;

    virtual bool	getSampleNames(BufferStringSet&) const
    			{ return false; }

};

/*!\brief reads from a 3D pre-stack seismic data store. */

class SeisPS3DReader : public SeisPSReader
{
public:

    virtual const PosInfo::CubeData&	posData() const			= 0;

};

/*!\brief reads from a 2D pre-stack seismic data store.

 Use the BinID's crl for trace number.

*/

class SeisPS2DReader : public SeisPSReader
{
public:

    virtual const PosInfo::Line2DData&	posData() const		= 0;

};


#endif

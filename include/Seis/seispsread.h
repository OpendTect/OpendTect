#ifndef seispsread_h
#define seispsread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsread.h,v 1.3 2005-01-05 15:06:57 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class BinID;
class IOPar;
class SeisTrcBuf;
namespace PosInfo { class CubeData; }


/*!\brief reads from a pre-stack seismic data store.

 Based on a BinID a gather may be available. No writing.

*/

class SeisPSReader
{
public:

    virtual		~SeisPSReader()					{}

    virtual void	usePar(const IOPar&)				{}

    virtual bool	getGather(const BinID&,SeisTrcBuf&) const	= 0;
    virtual const char*	errMsg() const					= 0;

    virtual const PosInfo::CubeData&	posData() const		= 0;

};


#endif

#ifndef seispsread_h
#define seispsread_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsread.h,v 1.2 2004-12-30 17:29:35 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class BinID;
class IOPar;
class SeisTrcBuf;


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

};


#endif

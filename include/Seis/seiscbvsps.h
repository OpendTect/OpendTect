#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.1 2004-12-30 11:28:25 bert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "bufstring.h"
class IOPar;


/*!\brief reads from a CBVS pre-stack seismic data store.

  Every inline is a CBVS cube. A gather represents one crossline. 
  Because CBSV seismics is inline-sorted, we the crossline number is stored
  as inline in the cube. upon retrieval actual BinID and Coord are restored.

 */

class SeisCBVSPSReader : public SeisPSReader
{
public:

    			SeisCBVSPSReader(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPSReader();

    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

protected:

    mutable BufferString errmsg_;
    const BufferString	dirnm_;
    const IOPar&	pars_;

};


#endif

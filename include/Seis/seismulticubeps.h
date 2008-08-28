#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seismulticubeps.h,v 1.1 2008-08-28 12:14:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
class SeisTrcReader;


/*!\brief PS data store reader based on multiple 3D CBVS cubes */

class MultiCubeSeisPSReader : public SeisPS3DReader
{
public:

    			MultiCubeSeisPSReader(const char* fnm);
			// Check errMsg() to see failure
			~MultiCubeSeisPSReader();

    SeisTrc*		getTrace(const BinID&,int) const;
    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const PosInfo::CubeData& posData() const	{ return posdata_; }
    bool		getSampleNames(BufferStringSet&) const
			{ return false; }

    void		usePar(const IOPar&);

protected:

    PosInfo::CubeData&		posdata_;
    ObjectSet<SeisTrcReader>	rdrs_;

};


#endif

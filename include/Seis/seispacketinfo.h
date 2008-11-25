#ifndef seispacketinfo_h
#define seispacketinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 1995 / Nov 2008
 RCS:		$Id: seispacketinfo.h,v 1.1 2008-11-25 11:37:46 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "bufstring.h"
#include "ranges.h"
class PosAuxInfo;
namespace PosInfo { class CubeData; }


/*!\brief Information for a packet of seismics, AKA tape header info */


class SeisPacketInfo
{
public:

			SeisPacketInfo()	{ clear(); }

    BufferString	usrinfo;
    BufferString	stdinfo;
    int			nr;
    bool		fullyrectandreg;

    const PosInfo::CubeData* cubedata;

    StepInterval<int>	inlrg;
    StepInterval<int>	crlrg;
    StepInterval<float>	zrg;
    bool		inlrev;
    bool		crlrev;

    void		clear();

    static const char*	sBinIDs;
    static const char*	sZRange;

    static BufferString	defaultusrinfo;

};


#endif

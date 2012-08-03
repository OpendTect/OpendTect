#ifndef seispacketinfo_h
#define seispacketinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 1995 / Nov 2008
 RCS:		$Id: seispacketinfo.h,v 1.4 2012-08-03 13:00:36 cvskris Exp $
________________________________________________________________________

-*/
 
#include "seismod.h"
#include "bufstring.h"
#include "ranges.h"
class PosAuxInfo;
namespace PosInfo { class CubeData; }


/*!\brief Information for a packet of seismics, AKA tape header info */


mClass(Seis) SeisPacketInfo
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


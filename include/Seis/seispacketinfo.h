#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
#include "seismod.h"
#include "bufstring.h"
#include "ranges.h"
class PosAuxInfo;
namespace PosInfo { class CubeData; }


/*!\brief Information for a packet of seismics, AKA tape header info */


mExpClass(Seis) SeisPacketInfo
{
public:
    static void		initClass();
    
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

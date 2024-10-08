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

			SeisPacketInfo();
    virtual		~SeisPacketInfo();
			mOD_DisableCopy(SeisPacketInfo)

    BufferString	usrinfo_;
    BufferString	stdinfo_;
    int			nr_					= 0;
    bool		fullyrectandreg_			= false;

    const PosInfo::CubeData* cubedata_				= nullptr;

    StepInterval<int>	inlrg_;
    StepInterval<int>	crlrg_;
    StepInterval<float> zrg_;
    bool		inlrev_					= false;
    bool		crlrev_					= false;

    void		clear();

    static const char*	sBinIDs();
    static const char*	sZRange();

    static BufferString defaultusrinfo_;

    mDeprecated("Use usrinfo_")
    BufferString&	usrinfo;
    mDeprecated("Use stdinfo_")
    BufferString&	stdinfo;
    mDeprecated("Use nr_")
    int&		nr;
    mDeprecated("Use fullyrectandreg")
    bool&		fullyrectandreg;

    mDeprecated("Use inlrg_")
    StepInterval<int>&	inlrg;
    mDeprecated("Use crlrg_")
    StepInterval<int>&	crlrg;
    mDeprecated("Use zrg_")
    StepInterval<float>& zrg;
    mDeprecated("Use inlrev_")
    bool&		inlrev;
    mDeprecated("Use crlrev_")
    bool&		crlrev;

};

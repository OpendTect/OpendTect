#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.2 2004-08-27 10:07:32 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class IOObj;
class MultiID;
class CtxtIOObj;
class CubeSampling;
template <class T> class StepInterval;


class uiSeisIOObjInfo
{
public:

    			uiSeisIOObjInfo(const IOObj&,bool error_feedback=true);
    			uiSeisIOObjInfo(const MultiID&,bool err_feedback=true);
			~uiSeisIOObjInfo();

    struct SpaceInfo
    {
			SpaceInfo(int ns=-1,int ntr=-1,int bps=4);
	int		expectednrsamps;
	int		expectednrtrcs;
	int		maxbytespsamp;
    };

    bool		is2D() const;
    bool		provideUserInfo() const;
    int			expectedMBs(const SpaceInfo&) const;
    bool		checkSpaceLeft(const SpaceInfo&) const;
    bool		getRanges(CubeSampling&) const;
    bool		getBPS(int& bps,int icomp=-1) const;
			//!< max bytes per sample, component -1 => add all

    static const char*	sKeyEstMBs;

protected:

    CtxtIOObj&		ctio;
    bool		doerrs;

};


#endif

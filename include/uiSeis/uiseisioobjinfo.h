#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.1 2004-07-01 15:14:43 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class IOObj;
class CtxtIOObj;
class MultiID;
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

    bool		provideUserInfo() const;
    int			expectedMBs(const SpaceInfo&) const;
    bool		checkSpaceLeft(const SpaceInfo&) const;
    bool		getRanges(StepInterval<int>& inlrg,
				  StepInterval<int>& crlrg,
				  StepInterval<float>& zrg) const;
    bool		getBPS(int& bps,int icomp=-1) const;
			//!< max bytes per sample, component -1 => add all

    static const char*	sKeyEstMBs;

protected:

    CtxtIOObj&		ctio;
    bool		doerrs;

};


#endif

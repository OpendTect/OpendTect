#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.3 2004-09-23 11:30:26 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class IOObj;
class MultiID;
class CtxtIOObj;
class CubeSampling;
class BufferStringSet;
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

    bool		isOK() const;
    bool		is2D() const;
    bool		provideUserInfo() const;
    int			expectedMBs(const SpaceInfo&) const;
    bool		checkSpaceLeft(const SpaceInfo&) const;
    bool		getRanges(CubeSampling&) const;
    bool		getBPS(int& bps,int icomp=-1) const;
			//!< max bytes per sample, component -1 => add all
    void		getAttribKeys(BufferStringSet&,bool add=true) const;
			//!< list of entries like: "100010.6 | En 60"
			//!< the attr is optional and is only filled for 2D

    static const char*	sKeyEstMBs;

protected:

    CtxtIOObj&		ctio;
    bool		doerrs;

};


#endif

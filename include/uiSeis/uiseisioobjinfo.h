#ifndef uiseisioobjinfo_h
#define uiseisioobjinfo_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseisioobjinfo.h,v 1.6 2004-10-07 11:27:25 bert Exp $
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
    			//!< For 3D: IOObj ID, for 2D: list of ID|attrnm

    			// 2D only
    void		getLineNames( BufferStringSet& b, bool add=true ) const
				{ getNms(b,add,false); }
    void		getAttribNames( BufferStringSet& b, bool add=true) const
				{ getNms(b,add,true); }
    void		getAttribNamesForLine( const char* nm,
					BufferStringSet& b, bool add=true) const
				{ getNmsSubSel(nm,b,add,false); }
    void		getLineNamesWithAttrib( const char* nm,
					BufferStringSet& b, bool add=true) const
				{ getNmsSubSel(nm,b,add,true); }

    static const char*	sKeyEstMBs;

protected:

    CtxtIOObj&		ctio;
    bool		doerrs;

    void		getNms(BufferStringSet&,bool,bool) const;
    void		getNmsSubSel(const char*,BufferStringSet&,bool,
	    			     bool) const;
};


#endif

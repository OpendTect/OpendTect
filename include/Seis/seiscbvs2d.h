#ifndef seiscbvs2d_h
#define seiscbvs2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seiscbvs2d.h,v 1.6 2004-10-20 16:12:33 bert Exp $
________________________________________________________________________

-*/

#include "seis2dline.h"
class SeisTrc;


class SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
{
public:

			SeisCBVS2DLineIOProvider();

    bool		isUsable(const IOPar&) const;
    bool		isEmpty(const IOPar&) const;

    bool		getGeometry(const IOPar&,Line2DGeometry&) const;
    Executor*		getFetcher(const IOPar&,SeisTrcBuf&,
	    			   const SeisSelData* sd=0);
    Seis2DLinePutter*	getReplacer(const IOPar&);
    Seis2DLinePutter*	getAdder(IOPar&,const IOPar*,const char*);

    bool		getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const;
    bool		getRanges(const IOPar&,StepInterval<int>&,
	    			  StepInterval<float>&) const;

    void		removeImpl(const IOPar&) const;

private:

    static int		factid;

};


#endif

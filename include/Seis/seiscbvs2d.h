#ifndef seiscbvs2d_h
#define seiscbvs2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seiscbvs2d.h,v 1.2 2004-08-23 16:12:39 bert Exp $
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

    Executor*		getFetcher(const IOPar&,SeisTrcBuf&,
	    			   const SeisSelData* sd=0);
    Executor*		getPutter(IOPar&,const SeisTrcBuf&,const IOPar*);
    bool		getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const;
    bool		getRanges(const IOPar&,StepInterval<int>&,
	    			  StepInterval<float>&) const;

    void		removeImpl(const IOPar&) const;

private:

    static int		factid;

};


#endif

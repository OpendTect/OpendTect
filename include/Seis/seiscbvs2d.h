#ifndef seiscbvs2d_h
#define seiscbvs2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seiscbvs2d.h,v 1.1 2004-06-18 13:58:07 bert Exp $
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

    Executor*		getFetcher(const IOPar&,SeisTrcBuf&);
    Executor*		getPutter(IOPar&,const SeisTrcBuf&,const IOPar*);

private:

    static int		factid;

};


#endif

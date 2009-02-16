#ifndef seiscbvs2d_h
#define seiscbvs2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id: seiscbvs2d.h,v 1.11 2009-02-16 17:17:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "seis2dline.h"
class SeisTrc;

namespace PosInfo { class Line2DData; }


mClass SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
{
public:

			SeisCBVS2DLineIOProvider();

    bool		isUsable(const IOPar&) const;
    bool		isEmpty(const IOPar&) const;

    bool		getGeometry(const IOPar&,PosInfo::Line2DData&) const;
    Executor*		getFetcher(const IOPar&,SeisTrcBuf&,int,
	    			   const Seis::SelData* sd=0);
    Seis2DLinePutter*	getReplacer(const IOPar&);
    Seis2DLinePutter*	getAdder(IOPar&,const IOPar*,const char*);

    bool		getTxtInfo(const IOPar&,BufferString&,
	    			   BufferString&) const;
    bool		getRanges(const IOPar&,StepInterval<int>&,
	    			  StepInterval<float>&) const;

    void		removeImpl(const IOPar&) const;

private:

    static int		factid;

public:

    static const char*	getFileName(const IOPar&);

};


#endif

#ifndef seiscbvs2d_h
#define seiscbvs2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "seis2dlineio.h"
class SeisTrc;
class CBVSSeisTrcTranslator;

namespace PosInfo { class Line2DData; }


mClass(Seis) SeisCBVS2DLineIOProvider : public Seis2DLineIOProvider
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


mClass(Seis) SeisCBVS2DLinePutter : public Seis2DLinePutter
{
public:

			SeisCBVS2DLinePutter(const char*,const IOPar&);
			~SeisCBVS2DLinePutter();

    const char*		errMsg() const      		{ return errmsg_.str();}
    int			nrWritten() const		{ return nrwr; }
    bool		put(const SeisTrc&);
    bool		close();

    int                 nrwr;
    BufferString        fname;
    BufferString        errmsg_;
    CBVSSeisTrcTranslator* tr;
    BinID               bid;
    DataCharacteristics::UserType preseldt;

};

#endif


#ifndef uiseiscbvsimpfromothersurv_h
#define uiseiscbvsimpfromothersurv_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
 RCS:           $Id: uiseiscbvsimpfromothersurv.h,v 1.1 2010-12-14 08:52:02 cvsbruno Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "binidvalset.h"
#include "executor.h"
#include "uidialog.h"

class BinIDValueSet;
class CBVSSeisTrcTranslator;
class CtxtIOObj;
class HorSampling;
class HorSamplingIterator;
class IOObj;
class RCol2Coord;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcInfo;
class uiSeisSel;


mClass SeisImpCBVSFromOtherSurvey : public Executor
{
public:

			SeisImpCBVSFromOtherSurvey(const IOObj&,IOObj&);
			~SeisImpCBVSFromOtherSurvey();

    const char*         message() const		{ return "Importing CBVS"; }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         nrDoneText() const      { return "Traces handled"; }

    int                 nextStep();

protected:

    const IOObj& 	inioobj_;
    IOObj&              outioobj_;

    SeisTrcWriter*      wrr_;
    CBVSSeisTrcTranslator* tr_;

    int                 nrdone_;
    mutable int         totnr_;
    BufferString        errmsg_;

    BinID		curbid_;
    HorSamplingIterator* hsit_;
    BinIDValueSet	bidset_;
    TypeSet<BinID> 	oldbids_;
    TypeSet<Coord> 	oldcoords_;
    float		oldinldist_;
    float		oldxlndist_;
    int			xzeropadfac_;
    int			yzeropadfac_;

    void		prepareRead();
    void		getCubeSamplingInNewSurvey(); 
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;

    bool                createTranslators();
    bool                createWriter();

    bool		findSquareTracesAroundCurbid(int,
					ObjectSet<SeisTrc>&) const;
    void		sincInterpol(int,ObjectSet<SeisTrc>&,int) const;
						
    float 		getInlXlnDist(const RCol2Coord&,bool) const;
};



mClass uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{
public:

			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*,const IOObj&);
			~uiSeisImpCBVSFromOtherSurveyDlg();
protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;

    uiSeisSel*          outfld_;

    bool		acceptOK(CallBacker*);
};


#endif

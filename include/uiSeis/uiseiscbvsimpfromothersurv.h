#ifndef uiseiscbvsimpfromothersurv_h
#define uiseiscbvsimpfromothersurv_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
 RCS:           $Id: uiseiscbvsimpfromothersurv.h,v 1.2 2010-12-16 13:09:43 cvsbruno Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "executor.h"
#include "uidialog.h"
#include "horsampling.h"

class BinIDValueSet;
class CBVSSeisTrcTranslator;
class CtxtIOObj;
class IOObj;
class RCol2Coord;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcInfo;

class uiGenInput;
class uiLabeledSpinBox;
class uiSeisSel;
class uiSeisSubSel;

mClass SeisImpCBVSFromOtherSurvey : public Executor
{
public:

    enum Interpol	{ Sinc, Nearest };

			SeisImpCBVSFromOtherSurvey(const IOObj&,IOObj&);
			~SeisImpCBVSFromOtherSurvey();

    const char*         message() const		{ return "Importing CBVS"; }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         nrDoneText() const      { return "Traces handled"; }
    od_int64		totalNr() const;

    int                 nextStep();
    bool		prepareRead();

    inline void 	setInterpol(Interpol i) { interpol_ = i; }
    inline void		setCellSize( int sz )	{ cellsize_ = sz; }

    HorSampling& 	horSampling() 		{ return hrg_; }

protected:

    const IOObj& 	inioobj_;
    IOObj&              outioobj_;

    SeisTrcWriter*      wrr_;
    CBVSSeisTrcTranslator* tr_;

    int                 nrdone_;
    mutable int         totnr_;
    BufferString        errmsg_;

    int			cellsize_;
    Interpol		interpol_;

    BinID		curbid_;
    BinID		curoldbid_;
    HorSampling 	hrg_;
    HorSampling 	oldhrg_;
    HorSamplingIterator* hsit_;
    int			xzeropadfac_;
    int			yzeropadfac_;
    ObjectSet<SeisTrc>	trcsset_;

    bool                createTranslators();
    bool                createWriter();

    bool		findSquareTracesAroundCurbid(ObjectSet<SeisTrc>&) const;
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;
    float 		getInlXlnDist(const RCol2Coord&,bool) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};



mClass uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{
public:

			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*);
			~uiSeisImpCBVSFromOtherSurveyDlg();
protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
    SeisImpCBVSFromOtherSurvey* import_;

    uiGenInput*		finpfld_;
    uiSeisSel*          outfld_;
    uiGenInput*		interpfld_;
    uiLabeledSpinBox*	cellsizefld_;
    uiSeisSubSel*	subselfld_;

    bool		acceptOK(CallBacker*);
    void		cubeSel(CallBacker*);
    void		interpSelDone(CallBacker*);
};


#endif

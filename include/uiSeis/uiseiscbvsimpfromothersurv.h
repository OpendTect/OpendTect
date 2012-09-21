#ifndef uiseiscbvsimpfromothersurv_h
#define uiseiscbvsimpfromothersurv_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiseismod.h"
#include "arrayndimpl.h"
#include "cubesampling.h"
#include "uidialog.h"
#include "executor.h"
#include "fourier.h"
#include "position.h"


class ArrayNDWindow;
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

mClass(uiSeis) SeisImpCBVSFromOtherSurvey : public Executor
{
public:

    enum Interpol	{ Sinc, Nearest };

			SeisImpCBVSFromOtherSurvey(const IOObj&);
			~SeisImpCBVSFromOtherSurvey();

    const char* 	message() const		{ return "Importing CBVS"; }
    od_int64 		nrDone() const          { return nrdone_; }
    const char* 	nrDoneText() const      { return "Traces handled"; }
    od_int64 		totalNr() const		{ return totnr_; }
    int 		nextStep();

    bool		prepareRead(const char*);
    void		setPars(Interpol&,int,const CubeSampling&);
    inline void		setOutput( IOObj& obj )	{ outioobj_ = &obj; }

    const CubeSampling& cubeSampling() const 	{ return data_.cs_; }

protected:

    const IOObj& 	inioobj_;
    IOObj*		outioobj_;
    SeisTrcWriter* 	wrr_;
    CBVSSeisTrcTranslator* tr_;

    int                 nrdone_;
    mutable int         totnr_;
    BufferString        errmsg_;
    const char*		fullusrexp_;

    Interpol		interpol_;

    mStruct(uiSeis) 		PosData
    {
			PosData()
			    : hsit_(0)
			    , cs_(false) {} 

	BinID		curbid_;
	CubeSampling 	cs_;
	HorSamplingIterator* hsit_;
    };
    PosData		data_, olddata_;

    int			padfac_;
    int			sz_;
    int			newsz_;
    int			szz_;

    Fourier::CC*	fft_;
    ObjectSet<SeisTrc>	trcsset_;
    Array3DImpl<float_complex>* arr_;
    Array3DImpl<float_complex>* fftarr_;
    ArrayNDWindow*	taper_;

    bool                createTranslators(const char*);
    bool                createWriter();

    bool		findSquareTracesAroundCurbid(ObjectSet<SeisTrc>&) const;
    void		getCubeInfo(TypeSet<Coord>&,TypeSet<BinID>&) const;
    float 		getInlXlnDist(const RCol2Coord&,bool,int) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};



mClass(uiSeis) uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{
public:

			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*);
			~uiSeisImpCBVSFromOtherSurveyDlg();
protected:

    CtxtIOObj&		inctio_;
    CtxtIOObj&		outctio_;
    SeisImpCBVSFromOtherSurvey* import_;
    bool		issinc_;
    SeisImpCBVSFromOtherSurvey::Interpol interpol_;

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



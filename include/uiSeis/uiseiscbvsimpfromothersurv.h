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
#include "trckeyzsampling.h"
#include "uidialog.h"
#include "executor.h"
#include "fourier.h"
#include "position.h"
#include "uistring.h"

class ArrayNDWindow;
class CBVSSeisTrcTranslator;
class CtxtIOObj;
class IOObj;
class SeisTrc;
class SeisTrcWriter;
class SeisTrcInfo;
namespace Pos { class IdxPair2Coord; }

class uiGenInput;
class uiLabeledSpinBox;
class uiSeisSel;
class uiSeisSubSel;

mExpClass(uiSeis) SeisImpCBVSFromOtherSurvey : public Executor
{ mODTextTranslationClass(SeisImpCBVSFromOtherSurvey);
public:

    enum Interpol	{ Sinc, Nearest };

			SeisImpCBVSFromOtherSurvey(const IOObj&);
			~SeisImpCBVSFromOtherSurvey();

    uiString		uiMessage() const	{ return tr("Importing CBVS"); }
    od_int64		nrDone() const          { return nrdone_; }
    uiString		uiNrDoneText() const	{ return tr("Traces handled"); }
    od_int64		totalNr() const		{ return totnr_; }
    int		nextStep();

    bool		prepareRead(const char*);
    void		setPars(Interpol&,int,const TrcKeyZSampling&);
    inline void		setOutput( IOObj& obj )	{ outioobj_ = &obj; }

    const TrcKeyZSampling& cubeSampling() const { return data_.tkzs_; }

protected:

    const IOObj&	inioobj_;
    IOObj*		outioobj_;
    SeisTrcWriter*	wrr_;
    CBVSSeisTrcTranslator* tr_;

    int                 nrdone_;
    mutable int         totnr_;
    uiString		errmsg_;
    const char*		fullusrexp_;

    Interpol		interpol_;

    mStruct(uiSeis)		PosData
    {
			PosData()
			    : hsit_(0)
			    , tkzs_(false) {}

	BinID		curbid_;
	TrcKeyZSampling tkzs_;
	TrcKeySamplingIterator* hsit_;
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
    float		getInlXlnDist(const Pos::IdxPair2Coord&,bool,int) const;
    SeisTrc*		readTrc(const BinID&) const;
    void		sincInterpol(ObjectSet<SeisTrc>&) const;
};



mExpClass(uiSeis) uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{ mODTextTranslationClass(uiSeisImpCBVSFromOtherSurveyDlg);
public:

			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*);

protected:

    SeisImpCBVSFromOtherSurvey* import_;
    SeisImpCBVSFromOtherSurvey::Interpol interpol_;
    bool		issinc_;

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



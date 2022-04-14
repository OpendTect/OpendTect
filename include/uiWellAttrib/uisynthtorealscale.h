#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uidialog.h"
#include "uistring.h"
#include "multiid.h"
#include "trckeysampling.h"

class SeisTrc;
class SeisTrcBuf;
class TaskRunner;
class uiSeisSel;
class uiIOObjSel;
class uiLabel;
class uiGenInput;
class uiStratSeisEvent;
class uiSynthToRealScaleStatsDisp;
template <class T> class ODPolygon;
namespace EM { class Horizon; class EMObjectIterator; }
namespace Strat { class SeisEvent; }
namespace StratSynth { class DataMgr; }


/*!\brief To determine scaling of synthetics using real data.

  Note: the input trc buf *must* have ref times in the trc.info().pick's.

 */

mExpClass(uiWellAttrib) uiSynthToRealScale : public uiDialog
{ mODTextTranslationClass(uiSynthToRealScale);
public:

			uiSynthToRealScale(uiParent*,bool is2d,
					   const StratSynth::DataMgr&,
					   const MultiID& wvltid);
			~uiSynthToRealScale();

    const MultiID&	inpWvltID() const	{ return inpwvltid_; }
    const MultiID&	selWvltID() const	{ return outwvltid_; }

protected:

    const bool		is2d_;
    const StratSynth::DataMgr&	stratsynth_;
    MultiID		inpwvltid_;
    MultiID		outwvltid_;

    ODPolygon<float>*	polygon_ = nullptr;
    TrcKeySampling	polyhs_;
    RefMan<EM::Horizon> horizon_;
    EM::EMObjectIterator* horiter_ = nullptr;
    Strat::SeisEvent&	seisev_;

    uiGenInput*		synthselfld_ = nullptr;
    uiSeisSel*		seisfld_ = nullptr;
    uiIOObjSel*		polyfld_ = nullptr;
    uiStratSeisEvent*	evfld_ = nullptr;
    uiIOObjSel*		horfld_ = nullptr;
    uiLabel*		valislbl_ = nullptr;
    uiSynthToRealScaleStatsDisp*	synthstatsfld_ = nullptr;
    uiSynthToRealScaleStatsDisp*	realstatsfld_ = nullptr;
    uiGenInput*		finalscalefld_ = nullptr;
    uiIOObjSel*		wvltfld_ = nullptr;

    void		setScaleFld(CallBacker*);
    void		goPush(CallBacker*);
    bool		acceptOK(CallBacker*);

    bool		getEvent();
    bool		getHorData(TaskRunner&);
    float		getTrcValue(const SeisTrc&,float) const;
    void		updSynthStats();
    void		updRealStats();

    friend class	uiSynthToRealScaleRealStatCollector;

};



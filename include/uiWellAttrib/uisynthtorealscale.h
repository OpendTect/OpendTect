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
#include "trckeysampling.h"
#include "multiid.h"

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


/*!\brief To determine scaling of synthetics using real data.
 
  Note: the input trc buf *must* have ref times in the trc.info().pick's.

 */

mExpClass(uiWellAttrib) uiSynthToRealScale : public uiDialog
{ mODTextTranslationClass(uiSynthToRealScale);
public:

			uiSynthToRealScale(uiParent*,bool is2d,
					   const SeisTrcBuf&,
					   const MultiID& wvltid,
					   const char* reflvlnm);
			~uiSynthToRealScale();

    const MultiID&	inpWvltID() const	{ return inpwvltid_; }
    const MultiID&	selWvltID() const	{ return outwvltid_; }

protected:

    bool		is2d_;
    const SeisTrcBuf&	synth_;
    MultiID		inpwvltid_;
    MultiID		outwvltid_;

    ODPolygon<float>*	polygon_;
    TrcKeySampling		polyhs_;
    EM::Horizon*	horizon_;
    EM::EMObjectIterator* horiter_;
    Strat::SeisEvent&	seisev_;

    uiSeisSel*		seisfld_;
    uiIOObjSel*		horfld_;
    uiIOObjSel*		polyfld_;
    uiIOObjSel*		wvltfld_;
    uiStratSeisEvent*	evfld_;
    uiGenInput*		finalscalefld_;
    uiLabel*		valislbl_;
    uiSynthToRealScaleStatsDisp*	synthstatsfld_;
    uiSynthToRealScaleStatsDisp*	realstatsfld_;

    void		initWin(CallBacker*);
    void		setScaleFld(CallBacker*);
    void		goPush( CallBacker* )
    			{ updSynthStats(); updRealStats(); }
    bool		acceptOK(CallBacker*);

    bool		getEvent();
    bool		getHorData(TaskRunner&);
    float		getTrcValue(const SeisTrc&,float) const;
    void		updSynthStats();
    void		updRealStats();

    friend class	uiSynthToRealScaleRealStatCollector;

};



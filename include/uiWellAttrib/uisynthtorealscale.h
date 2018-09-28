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
#include "dbkey.h"
#include "stratsynthdatamgr.h"
#include "trckeysampling.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;
class TaskRunnerProvider;
class uiComboBox;
class uiLabel;
class uiGenInput;
class uiSeisSel;
class uiSeisSubSel;
class uiStratLevelHorSel;
class uiStratSeisEvent;
class uiSynthToRealScaleStatsDisp;
class uiWaveletIOObjSel;
namespace EM { class Horizon; class ObjectIterator; }
namespace Strat { class SeisEvent; }
namespace Seis { class SelData; }


/*!\brief To determine scaling of synthetics using real data. */

mExpClass(uiWellAttrib) uiSynthToRealScale : public uiDialog
{ mODTextTranslationClass(uiSynthToRealScale);
public:

    typedef StratSynth::DataMgr	DataMgr;
    typedef Strat::SeisEvent	SeisEvent;
    typedef Strat::Level::ID	LevelID;
    typedef DataMgr::SynthID	SynthID;

			uiSynthToRealScale(uiParent*,const DataMgr&,
					   const DBKey& wvltid,bool use2dseis,
					   const LevelID& deflvl);
			~uiSynthToRealScale();

    const DBKey&	inpWvltID() const	{ return inpwvltid_; }
    const DBKey&	scaledWvltID() const	{ return outwvltid_; }

protected:

    ConstRefMan<DataMgr> datamgr_;
    const DBKey		inpwvltid_;
    const bool		use2dseis_;
    DBKey		outwvltid_;
    Seis::SelData*	seldata_;
    TrcKeySampling	polyhs_;
    EM::Horizon*	horizon_;
    EM::ObjectIterator* horiter_;
    SeisEvent&		seisev_;
    BufferStringSet	synthnms_;

    uiSeisSel*		seisfld_;
    uiSeisSubSel*	seissubselfld_;
    uiComboBox*		synthfld_		= 0;
    uiStratLevelHorSel*	lvlhorfld_;
    uiWaveletIOObjSel*	wvltfld_;
    uiStratSeisEvent*	evfld_;
    uiGenInput*		finalscalefld_;
    uiLabel*		valislbl_;
    uiSynthToRealScaleStatsDisp* synthstatsfld_;
    uiSynthToRealScaleStatsDisp* realstatsfld_;

    void		initWin(CallBacker*);
    void		statsUsrValChgCB(CallBacker*);
    void		seisSelCB(CallBacker*);
    void		synthSelCB(CallBacker*);
    void		evChgCB(CallBacker*);
    void		levelSelCB(CallBacker*);
    void		subselChgCB(CallBacker*);
    void		goPushCB( CallBacker* )
			{ updSynthStats(); updRealStats(); }
    bool		acceptOK();

    SynthID		synthID() const;
    bool		getEvent();
    bool		getHorData(TaskRunnerProvider&);
    const SeisTrcBuf&	synthTrcBuf(SynthID) const;
    float		getTrcValue(const SeisTrc&,float) const;
    void		updSynthStats();
    void		updRealStats();
    void		setScaleFld();

    friend class	uiSynthToRealScaleRealStatCollector;

};

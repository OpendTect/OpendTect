#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

namespace EM { class Horizon; }

class uiHorSaveFieldGrp;
class Executor;
class uiGenInput;
class uiIOObjSel;
template <class T> class Array2D;

/*!\brief Base class for surface changers. At the moment only does horizons. */

mExpClass(uiEarthModel) uiChangeHorizonDlg : public uiDialog
{ mODTextTranslationClass(uiChangeHorizonDlg);
public:
				uiChangeHorizonDlg(uiParent*,EM::Horizon*,
						   bool is2d,const uiString&);
				~uiChangeHorizonDlg();

    uiHorSaveFieldGrp*		saveFldGrp() const { return savefldgrp_; }
    Notifier<uiChangeHorizonDlg> horReadyForDisplay;

protected:

    uiHorSaveFieldGrp*		savefldgrp_;
    uiIOObjSel*			inputfld_;
    uiGroup*			parsgrp_;

    EM::Horizon*		horizon_;
    bool			is2d_;

    bool			acceptOK(CallBacker*) override;
    bool			readHorizon();
    bool			doProcessing();
    bool			doProcessing2D();
    bool			doProcessing3D();

    void			attachPars();	//!< To be called by subclass
    virtual const char*		infoMsg(const Executor*) const	{ return 0; }
    virtual Executor*		getWorker(Array2D<float>&,
					  const StepInterval<int>&,
					  const StepInterval<int>&) = 0;
    virtual bool		fillUdfsOnly() const	{ return false;}
    virtual bool		needsFullSurveyArray() const { return false;}
    virtual const char*		undoText() const		{ return 0; }
};


class uiStepOutSel;

mExpClass(uiEarthModel) uiFilterHorizonDlg : public uiChangeHorizonDlg
{ mODTextTranslationClass(uiFilterHorizonDlg)
public:
				uiFilterHorizonDlg(uiParent*,EM::Horizon*);
				~uiFilterHorizonDlg();

protected:

    uiGenInput*			medianfld_;
    uiStepOutSel*		stepoutfld_;

    Executor*		getWorker(Array2D<float>&,
				  const StepInterval<int>&,
				  const StepInterval<int>&) override;
    const char*		undoText() const override { return "filtering"; }

};

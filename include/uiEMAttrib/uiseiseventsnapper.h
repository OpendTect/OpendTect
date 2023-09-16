#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"

namespace EM { class Horizon; }

class IOObj;
class uiGenInput;
class uiHorSaveFieldGrp;
class uiIOObjSel;
class uiSeisSel;

/*! \brief Part Server for Wells */

mExpClass(uiEMAttrib) uiSeisEventSnapper : public uiDialog
{ mODTextTranslationClass(uiSeisEventSnapper);
public:
			uiSeisEventSnapper(uiParent*,const IOObj*,bool is2d);
			~uiSeisEventSnapper();

    uiHorSaveFieldGrp*	saveFldGrp() const	{ return savefldgrp_; }
    Notifier<uiSeisEventSnapper>	readyForDisplay;

protected:

    uiHorSaveFieldGrp*	savefldgrp_;
    uiIOObjSel*		horinfld_;
    uiSeisSel*		seisfld_;
    uiGenInput*		eventfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		undefpolicyfld_;
    bool		is2d_;

    bool		acceptOK(CallBacker*) override;
    bool		readHorizon();

    RefMan<EM::Horizon> horizon_;

};

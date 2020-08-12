#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"

namespace EM { class Horizon; }

class CtxtIOObj;
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
    bool		is2d_;

    virtual bool	acceptOK(CallBacker*);
    bool		readHorizon();

    EM::Horizon*	horizon_;

};


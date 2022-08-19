#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"
#include "emposid.h"

class IOObj;
class uiGenInput;
class uiIOObjSel;
class uiT2DConvSel;


mClass(uiGrav) uiGravHorCalc : public uiDialog
{ mODTextTranslationClass(uiGravHorCalc);
public:

			uiGravHorCalc(uiParent*,EM::ObjectID);
			~uiGravHorCalc();

protected:

    const MultiID	horid_;
    const IOObj*	horioobj_;

    uiIOObjSel*		topfld_;
    uiIOObjSel*		botfld_;
    uiGenInput*		denvarfld_;
    uiGenInput*		denvaluefld_;
    uiGenInput*		denattrfld_;
    uiGenInput*		cutoffangfld_;
    uiGenInput*		attrnmfld_;
    uiT2DConvSel*	t2dfld_;

    void		initFlds(CallBacker*);
    void		denVarSel(CallBacker*);
    void		topSel(CallBacker*);

    bool		acceptOK(CallBacker*);

};

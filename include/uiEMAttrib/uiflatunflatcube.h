#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"

#include "uidialog.h"
#include "multiid.h"

class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeisSubSel;


/*! \brief Create flattened cube from horizon */

mClass(uiEMAttrib) uiFlatUnflatCube : public uiDialog
{
mODTextTranslationClass(uiFlatUnflatCube)
public:
			uiFlatUnflatCube(uiParent*);
			~uiFlatUnflatCube();

    void		setHorizon(const MultiID&);

protected:

    bool		doFlatten() const;
    uiSeisSel*		getInpFld();
    uiSeisSel*		getOutpFld();

    void		finalizeCB(CallBacker*);
    void		modeChgCB(CallBacker*);
    void		inpSelCB(CallBacker*);
    void		horSelCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiGenInput*		modefld_;
    uiSeisSel*		seisinfld_;
    uiSeisSel*		flatinfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		flatvalfld_;
    uiSeisSubSel*	rgfld_;
    uiSeisSel*		flatoutfld_;
    uiSeisSel*		seisoutfld_;
};

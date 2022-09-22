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

    void		finalizeCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		inpSelCB(CallBacker*);
    void		horSelCB(CallBacker*);

    uiGenInput*		modefld_;
    uiSeisSel*		seisinfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		flatvalfld_;
    uiSeisSubSel*	rgfld_;
    uiSeisSel*		seisoutfld_;
};

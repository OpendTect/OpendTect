#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiGenInput;
class uiSpinBox;
namespace visBase{ class SceneColTab; }

mExpClass(uiVis) uiSceneColorbarMgr : public uiDialog
{ mODTextTranslationClass(uiSceneColorbarMgr);
public:
			uiSceneColorbarMgr(uiParent*,visBase::SceneColTab*);
			~uiSceneColorbarMgr();

protected:
    visBase::SceneColTab* scenecoltab_;
    uiSpinBox*		widthfld_;
    uiSpinBox*		heightfld_;
    uiGenInput*		posfld_;

    void		sizeChangedCB(CallBacker*);
    void		posChangedCB(CallBacker*);
};

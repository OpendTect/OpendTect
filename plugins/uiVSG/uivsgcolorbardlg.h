#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgmod.h"

#include "uidialog.h"

class uiGenInput;
class uiSpinBox;
class uiVSGViewer;

/*!\brief The size and the position of the color bar of the VSG viewer.
  Counterpart of uiSceneColorbarMgr. A change is applied at once. */

mExpClass(uiVSG) uiVSGColorBarDlg : public uiDialog
{ mODTextTranslationClass(uiVSGColorBarDlg);
public:
			uiVSGColorBarDlg(uiParent*,uiVSGViewer&);
			~uiVSGColorBarDlg();

private:

    void		sizeChangedCB(CallBacker*);
    void		posChangedCB(CallBacker*);
    void		setSizeLimits();

    uiVSGViewer&	viewer_;
    uiSpinBox*		widthfld_;
    uiSpinBox*		heightfld_;
    uiGenInput*		posfld_;
};

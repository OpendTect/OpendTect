#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidpsaddcolumndlg.h"
#include "bufstringset.h"

class uiCheckBox;
class uiColorTableGroup;
class uiDataPointSetCrossPlotter;
class uiGenInput;


mExpClass(uiIo) uiDPSSelectednessDlg : public uiDPSAddColumnDlg
{ mODTextTranslationClass(uiDPSSelectednessDlg);
public:
				uiDPSSelectednessDlg(uiParent*,
					uiDataPointSetCrossPlotter&);
    bool			acceptOK(CallBacker*) override;

protected:

    void			addColumn();
    void			showOverlayAttrib();
    void			showIn3DScene();

    void			showOverlayClicked(CallBacker*);
    void			show3DSceneClicked(CallBacker*);

    uiCheckBox*			showoverlayfld_;
    uiCheckBox*			showin3dscenefld_;
    uiColorTableGroup*		coltabfld_;
    uiGenInput*			selaxisfld_;
    uiDataPointSetCrossPlotter&	plotter_;

};

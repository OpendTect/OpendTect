#pragma once

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidpsaddcolumndlg.h"

class uiCheckBox;
class uiColTabSel;
class uiDataPointSetCrossPlotter;
class uiGenInput;


mExpClass(uiIo) uiDPSSelectednessDlg : public uiDPSAddColumnDlg
{ mODTextTranslationClass(uiDPSSelectednessDlg);
public:
				uiDPSSelectednessDlg(uiParent*,
					uiDataPointSetCrossPlotter&);
    bool			acceptOK();

protected:

    void			addColumn();
    void			showOverlayAttrib();
    void			showIn3DScene();

    void			showOverlayClicked(CallBacker*);
    void			show3DSceneClicked(CallBacker*);

    uiCheckBox*			showoverlayfld_;
    uiCheckBox*			showin3dscenefld_;
    uiColTabSel*		coltabfld_;
    uiGenInput*			selaxisfld_;
    uiDataPointSetCrossPlotter&	plotter_;

};

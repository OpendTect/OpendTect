#ifndef uidpsselectednessdlg_h
#define uidpsselectednessdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
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
    bool			acceptOK();

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


#endif

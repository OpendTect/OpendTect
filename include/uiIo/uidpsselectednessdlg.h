#ifndef uidpsselectednessdlg_h
#define uidpsselectednessdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
 RCS:           $Id$: 
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidpsaddcolumndlg.h"
#include "bufstringset.h"

class uiCheckBox;
class uiColorTable;
class uiDataPointSetCrossPlotter;
class uiGenInput;
class uiTable;

class MathExpression;

mExpClass(uiIo) uiDPSSelectednessDlg : public uiDPSAddColumnDlg
{
public:
    				uiDPSSelectednessDlg(uiParent*,
					uiDataPointSetCrossPlotter&);
    bool			acceptOK(CallBacker*);

protected:

    void			addColumn();
    void			showOverlayAttrib();
    void			showIn3DScene();

    void			showOverlayClicked(CallBacker*);
    void			show3DSceneClicked(CallBacker*);
    
    uiCheckBox*			showoverlayfld_;
    uiCheckBox*			showin3dscenefld_;
    uiColorTable*		coltabfld_;
    uiGenInput*			selaxisfld_;
    uiDataPointSetCrossPlotter&	plotter_;
};

#endif


#ifndef uidpsselectednessdlg_h
#define uidpsselectednessdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
 RCS:           $Id: uidpsselectednessdlg.h,v 1.1 2011/07/11 11:40:57 cvssatyaki Exp $: 
________________________________________________________________________

-*/

#include "uidpsaddcolumndlg.h"
#include "bufstringset.h"

class uiCheckBox;
class uiColorTable;
class uiDataPointSetCrossPlotter;
class uiGenInput;
class uiTable;

class MathExpression;

mClass uiDPSSelectednessDlg : public uiDPSAddColumnDlg
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

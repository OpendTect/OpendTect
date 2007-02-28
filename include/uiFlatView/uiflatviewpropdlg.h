#ifndef uiflatviewpropdlg_h
#define uiflatviewpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.h,v 1.1 2007-02-28 15:58:44 cvshelene Exp $
________________________________________________________________________

-*/

#include "colortab.h"
#include "flatview.h"
#include "uidlggroup.h"

class uiLabel;
class uiGenInput;
class uiColorInput;
class ColorTableEditor;

    
/*!\brief flat viewer properties tabs */

class uiFlatViewPropTab : public uiDlgGroup
{
public:

    void		fillDispPars(FlatView::DataDispPars::Common&) const;

protected:
    			uiFlatViewPropTab(uiParent*,
					const FlatView::DataDispPars::Common&,
					const char*);

    uiGenInput*		useclipfld_;
    uiGenInput*		clipratiofld_;
    uiGenInput*		rgfld_;
    uiGenInput*		blockyfld_;

    void		clipSel(CallBacker*);
};

		     
class uiWVAFVPropTab : public uiFlatViewPropTab
{
public:
    			uiWVAFVPropTab(uiParent*,
				    const FlatView::DataDispPars::WVA&,bool);

    bool                acceptOK();
    void		fillDispPars();
    bool		dispwva_;
    
    FlatView::DataDispPars::WVA		wvapars_;

protected:

    uiGenInput*		dispfld_;
    uiGenInput*		overlapfld_;
    uiGenInput*		midlinefld_;
    uiGenInput*		midvalfld_;
    uiColorInput*       wigcolsel_;
    uiColorInput*       midlcolsel_;
    uiColorInput*       leftcolsel_;
    uiColorInput*       rightcolsel_;

    void		dispSel(CallBacker*);
    void		midlineSel(CallBacker*);
};

class uiVDFVPropTab : public uiFlatViewPropTab
{
public:
    			uiVDFVPropTab(uiParent*,
				    const FlatView::DataDispPars::VD&,bool);

    bool                acceptOK();
    void		fillDispPars();
    bool		dispvd_;
    
    FlatView::DataDispPars::VD		vdpars_;

protected:

    uiGenInput*		dispfld_;
    ColorTableEditor*	coltabfld_;
    uiLabel*		coltablbl_;
    ColorTable		ctab_;

    void		clipSel(CallBacker*);
    void		dispSel(CallBacker*);
};


class uiFlatViewPropDlg : public uiTabStackDlg
{
public:
			uiFlatViewPropDlg(uiParent*,
					const FlatView::DataDispPars&,
					const CallBack& applcb);
    uiWVAFVPropTab*	wvaproptab_;
    uiVDFVPropTab*	vdproptab_;

};

#endif

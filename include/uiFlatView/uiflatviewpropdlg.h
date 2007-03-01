#ifndef uiflatviewpropdlg_h
#define uiflatviewpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.h,v 1.3 2007-03-01 19:35:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "colortab.h"
#include "flatview.h"
#include "uidlggroup.h"

class uiCheckBox;
class uiLabel;
class uiGenInput;
class uiColorInput;
class ColorTableEditor;

    
/*!\brief flat viewer properties tabs */

class uiFlatViewPropTab : public uiDlgGroup
{
public:

    virtual void	putToScreen()		= 0;
    virtual void	getFromScreen()		= 0;

    bool		acceptOK()		{ getFromScreen(); return true;}

protected:
    			uiFlatViewPropTab(uiParent*,FlatView::Viewer&,
					  const char*);

    FlatView::Viewer&	vwr_;
    FlatView::Context&	ctxt_;

};
    
/*!\brief flat viewer properties tabs */

class uiFlatViewDataDispPropTab : public uiFlatViewPropTab
{
public:

    virtual FlatView::DataDispPars::Common& commonDDPars()	= 0;

protected:
    			uiFlatViewDataDispPropTab(uiParent*,
					  FlatView::Viewer&,
					  const char*);
    FlatView::DataDispPars& ddpars_;

    uiGenInput*		useclipfld_;
    uiGenInput*		clipratiofld_;
    uiGenInput*		rgfld_;
    uiGenInput*		blockyfld_;

    void		clipSel(CallBacker*);
    virtual void	afterClipSelection()		{}
    void		setDoDisplay(bool);
    bool		dodisplay_;

    void		putCommonToScreen();
    void		getCommonFromScreen();
};

		     
class uiFVWVAPropTab : public uiFlatViewDataDispPropTab
{
public:
    			uiFVWVAPropTab(uiParent*,FlatView::Viewer&);

    FlatView::DataDispPars::Common& commonDDPars()	{ return pars_; }

    virtual void	putToScreen();
    virtual void	getFromScreen();

protected:

    FlatView::DataDispPars::WVA& pars_;

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


class uiFVVDPropTab : public uiFlatViewDataDispPropTab
{
public:
    			uiFVVDPropTab(uiParent*,FlatView::Viewer&);

    FlatView::DataDispPars::Common& commonDDPars()	{ return pars_; }

    virtual void	putToScreen();
    virtual void	getFromScreen();

protected:

    FlatView::DataDispPars::VD&		pars_;
    ColorTable		ctab_;

    uiGenInput*		dispfld_;
    ColorTableEditor*	coltabfld_;
    uiLabel*		coltablbl_;

    virtual void	afterClipSelection();
    void		dispSel(CallBacker*);
};


class uiFVAnnotPropTab : public uiFlatViewPropTab
{
public:

    			uiFVAnnotPropTab(uiParent*,FlatView::Viewer&);

    virtual void	putToScreen();
    virtual void	getFromScreen();

protected:
    
    FlatView::Annotation& annot_;

    class AxesGroup : public uiGroup
    {
    public:
			AxesGroup(uiParent*,FlatView::Annotation::AxisData&);

	void		putToScreen();
	void		getFromScreen();

    protected:

	FlatView::Annotation::AxisData&	ad_;

	uiGenInput*	namefld_;
	uiCheckBox*	showannotfld_;
	uiCheckBox*	showgridlinesfld_;
	uiCheckBox*	reversedfld_;

    };

    AxesGroup*		x1_;
    AxesGroup*		x2_;

};

		     
class uiFlatViewPropDlg : public uiTabStackDlg
{
public:
			uiFlatViewPropDlg(uiParent*,FlatView::Viewer&,
					  const CallBack& applcb);

    void		putAllToScreen();
    void		getAllFromScreen();

protected:

    uiFVWVAPropTab*	wvatab_;
    uiFVVDPropTab*	vdtab_;
    uiFVAnnotPropTab*	annottab_;

    CallBack		applycb_;
    void		doApply(CallBacker*);

};

#endif

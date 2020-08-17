#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "coltabsequence.h"
#include "flatview.h"
#include "uidlggroup.h"
#include "uistring.h"

class uiLabel;
class uiGenInput;
class uiComboBox;
class uiCheckBox;
class uiColorInput;
class uiSelLineStyle;
class uiColorTableGroup;
class uiSelLineStyle;

/*!
\brief uiFlatViewer properties tabs.
*/

mExpClass(uiFlatView) uiFlatViewPropTab : public uiDlgGroup
{ mODTextTranslationClass(uiFlatViewPropTab);
public:

    virtual void	putToScreen()		= 0;

protected:
			uiFlatViewPropTab(uiParent*,FlatView::Viewer&,
					  uiString);

    FlatView::Viewer&	vwr_;
    FlatView::Appearance& app_;
    DataPackMgr&	dpm_;

};


/*!
\brief uiFlatViewer data display properties tabs.
*/

mExpClass(uiFlatView) uiFlatViewDataDispPropTab : public uiFlatViewPropTab
{ mODTextTranslationClass(uiFlatViewDataDispPropTab);
public:

    void		setDataNames();
    virtual void	setData()			= 0;
    bool		doDisp() const;

protected:
			uiFlatViewDataDispPropTab(uiParent*,FlatView::Viewer&,
				const uiString&,bool showdisplayfield=true);

    FlatView::DataDispPars&	ddpars_;
    virtual FlatView::DataDispPars::Common& commonPars()	= 0;
    virtual BufferString	dataName() const		= 0;

    uiComboBox*		dispfld_;
    uiGenInput*		useclipfld_;
    uiGenInput*		symclipratiofld_;
    uiGenInput*		assymclipratiofld_;
    uiGenInput*		symmidvalfld_;
    uiGenInput*		usemidvalfld_;
    uiGenInput*		rgfld_;
    uiGenInput*		blockyfld_;
    bool		showdisplayfield_;

    uiObject*		lastcommonfld_;

    void		updateNonclipRange(CallBacker*);
    void		useMidValSel(CallBacker*);
    void		dispSel(CallBacker*);
    void		clipSel(CallBacker*);
    virtual void	handleFieldDisplay(bool)	= 0;

    void		putCommonToScreen();
    bool		acceptOK();
    void		doSetData(bool);

};


/*!
\brief uiFlatViewer WVA display properties tabs.
*/

mExpClass(uiFlatView) uiFVWVAPropTab : public uiFlatViewDataDispPropTab
{ mODTextTranslationClass(uiFVWVAPropTab);
public:
			uiFVWVAPropTab(uiParent*,FlatView::Viewer&);
			~uiFVWVAPropTab();

    virtual void	putToScreen();
    bool		acceptOK();
    virtual void	setData()		{ doSetData(true); }

protected:

    FlatView::DataDispPars::WVA& pars_;
    virtual FlatView::DataDispPars::Common& commonPars() { return pars_; }
    virtual BufferString	 dataName() const;

    uiGenInput*		overlapfld_;
    uiGenInput*		reflinefld_;
    uiGenInput*		refvalfld_;
    uiColorInput*       wigcolsel_;
    uiColorInput*	reflcolsel_;
    uiColorInput*       leftcolsel_;
    uiColorInput*       rightcolsel_;

    virtual void	handleFieldDisplay(bool);
    void		dispSel(CallBacker*);
    void		reflineSel(CallBacker*);
    void		dispChgCB(CallBacker*);
};


/*!
\brief uiFlatViewer VD display properties tabs.
*/

mExpClass(uiFlatView) uiFVVDPropTab : public uiFlatViewDataDispPropTab
{ mODTextTranslationClass(uiFVVDPropTab);
public:
			uiFVVDPropTab(uiParent*,FlatView::Viewer&);
			~uiFVVDPropTab();

    virtual void	putToScreen();
    virtual bool	acceptOK();
    virtual void	setData()		{ doSetData(false); }

protected:

    FlatView::DataDispPars::VD&		pars_;
    ColTab::Sequence			ctab_;
    virtual FlatView::DataDispPars::Common& commonPars() { return pars_; }
    virtual BufferString		dataName() const;

    uiColorTableGroup*	uicoltab_;
    uiLabel*		uicoltablbl_;

    virtual void	handleFieldDisplay(bool);
    void		dispSel(CallBacker*);
    void		dispChgCB(CallBacker*);
};


/*!
\brief uiFlatViewer annotation properties tabs.
*/

mExpClass(uiFlatView) uiFVAnnotPropTab : public uiFlatViewPropTab
{ mODTextTranslationClass(uiFVAnnotPropTab);
public:

			uiFVAnnotPropTab(uiParent*,FlatView::Viewer&,
					 const BufferStringSet* annots);
			~uiFVAnnotPropTab();

    virtual void	putToScreen();
    virtual bool	acceptOK();

    int			getSelAnnot() const	{ return x1_->getSelAnnot(); }
    void		setSelAnnot( int i )	{ x1_->setSelAnnot( i ); }
    void		fillPar(IOPar&) const;

protected:

    void		annotChgdCB(CallBacker*);
    void		auxNmFldCB(CallBacker*);
    void		getFromAuxFld(int);
    void		updateAuxFlds(int);

    FlatView::Annotation& annot_;

    mExpClass(uiFlatView) AxesGroup : public uiGroup
    { mODTextTranslationClass(AxesGroup)
    public:
			AxesGroup(uiParent*,OD::Orientation,
				  FlatView::Annotation::AxisData&,
				  const BufferStringSet* annots=nullptr,
				  bool dorevert=true);

	void		putToScreen();
	void		getFromScreen();

	int		getSelAnnot() const;
	void		setSelAnnot(int);
    protected:

	FlatView::Annotation::AxisData&	ad_;

	uiCheckBox*	showannotfld_;
	uiCheckBox*	showgridlinesfld_;
	uiCheckBox*	showauxannotfld_;
	uiSelLineStyle*	auxlinestylefld_;
	uiLabel*	auxlblfld_;
	uiCheckBox*	reversedfld_;
	uiGenInput*	annotselfld_;

	void		showAuxCheckedCB(CallBacker*);
	void		showAuxLineCheckedCB(CallBacker*);

    };

    uiColorInput*       colfld_;
    uiGenInput*		viewnrdeczfld_;
    AxesGroup*		x1_;
    AxesGroup*		x2_;

    uiGenInput*		auxnamefld_;
    uiSelLineStyle*	linestylefld_;
    uiSelLineStyle*	linestylenocolorfld_;
    uiColorInput*	fillcolorfld_;
    uiGenInput*		x1rgfld_;
    uiGenInput*		x2rgfld_;

    ObjectSet<FlatView::AuxData::EditPermissions>	permissions_;
    BoolTypeSet						enabled_;
    TypeSet<OD::LineStyle>				linestyles_;
    TypeSet<int>					indices_;
    TypeSet<Color>					fillcolors_;
    TypeSet<MarkerStyle2D>				markerstyles_;
    TypeSet<Interval<double> >				x1rgs_;
    TypeSet<Interval<double> >				x2rgs_;
    int							currentaux_;
};



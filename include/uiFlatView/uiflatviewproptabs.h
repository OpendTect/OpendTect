#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiFlatViewPropTab();

    virtual void	putToScreen()			= 0;
    virtual void	fillCommonPar(IOPar&) const	{}

protected:
			uiFlatViewPropTab(uiParent*,FlatView::Viewer&,
					  const uiString&);

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
			~uiFlatViewDataDispPropTab();

    void		setDataName(bool);
    void		setDataNames();
    virtual void	setData()			= 0;
    bool		doDisp() const;

protected:
			uiFlatViewDataDispPropTab(uiParent*,FlatView::Viewer&,
				const uiString&,bool showdisplayfield=true);

    FlatView::DataDispPars&	ddpars_;
    virtual FlatView::DataDispPars::Common& commonPars()	= 0;
    virtual BufferString	dataName() const		= 0;

    uiComboBox*		dispfld_		= nullptr;
    uiGenInput*		useclipfld_;
    uiGenInput*		symclipratiofld_;
    uiGenInput*		assymclipratiofld_;
    uiGenInput*		symmidvalfld_;
    uiGenInput*		usemidvalfld_;
    uiGenInput*		rgfld_;
    uiGenInput*		blockyfld_		= nullptr;
    bool		showdisplayfield_;

    uiObject*		lastcommonfld_;

    void		updateNonclipRange(CallBacker*);
    void		useMidValSel(CallBacker*);
    void		dispSel(CallBacker*);
    void		clipSel(CallBacker*);
    virtual void	handleFieldDisplay(bool)	= 0;

    void		putCommonToScreen();
    void		fillCommonPar(IOPar&) const override;
    bool		acceptOK() override;
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

    void		putToScreen() override;
    void		fillCommonPar(IOPar&) const override;
    bool		acceptOK() override;
    void		setData() override		{ doSetData(true); }

protected:

    FlatView::DataDispPars::WVA& pars_;
    FlatView::DataDispPars::Common& commonPars() override { return pars_; }
    BufferString	 dataName() const override;

    uiGenInput*		overlapfld_;
    uiGenInput*		reflinefld_;
    uiGenInput*		refvalfld_;
    uiColorInput*       wigcolsel_;
    uiColorInput*	reflcolsel_;
    uiColorInput*       leftcolsel_;
    uiColorInput*       rightcolsel_;

    void		handleFieldDisplay(bool) override;
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

    void		putToScreen() override;
    void		fillCommonPar(IOPar&) const override;
    bool		acceptOK() override;
    void		setData() override		{ doSetData(false); }

protected:

    FlatView::DataDispPars::VD&		pars_;
    ColTab::Sequence			ctab_;
    FlatView::DataDispPars::Common& commonPars() override { return pars_; }
    BufferString			dataName() const override;

    uiColorTableGroup*	uicoltab_;
    uiLabel*		uicoltablbl_;

    void		handleFieldDisplay(bool) override;
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
					 const uiStringSet* annotsdim0,
					 const uiStringSet* annotsdim1,
					 int selannotdim0,int selannotdim1);
			~uiFVAnnotPropTab();

    void		putToScreen() override;
    void		fillCommonPar(IOPar&) const override;
    bool		acceptOK() override;

    int			getSelAnnot(bool dim0) const;
    void		setSelAnnot(int i,bool dim0);
    int			nrZDecimals() const;
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
				  const uiStringSet* annots=nullptr,
				  int selannotdim=-1,bool dorevert=true);
			~AxesGroup();

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
	uiCheckBox*	reversedfld_	= nullptr;
	uiGenInput*	annotselfld_	= nullptr;

	void		showAuxCheckedCB(CallBacker*);
	void		showAuxLineCheckedCB(CallBacker*);

    };

    uiColorInput*       colfld_;
    AxesGroup*		x1_;
    AxesGroup*		x2_;
    uiGenInput*		viewnrdeczfld_	= nullptr;

    uiGenInput*		auxnamefld_	= nullptr;
    uiSelLineStyle*	linestylefld_;
    uiSelLineStyle*	linestylenocolorfld_;
    uiColorInput*	fillcolorfld_;
    uiGenInput*		x1rgfld_;
    uiGenInput*		x2rgfld_;

    ObjectSet<FlatView::AuxData::EditPermissions>	permissions_;
    BoolTypeSet						enabled_;
    TypeSet<OD::LineStyle>				linestyles_;
    TypeSet<int>					indices_;
    TypeSet<OD::Color>					fillcolors_;
    TypeSet<MarkerStyle2D>				markerstyles_;
    TypeSet<Interval<double> >				x1rgs_;
    TypeSet<Interval<double> >				x2rgs_;
    int							currentaux_;

public:
			mDeprecated("Provide dimension")
    int			getSelAnnot() const	{ return getSelAnnot(true); }
			mDeprecated("Provide dimension")
    void		setSelAnnot( int i )	{ setSelAnnot(i,true); }
};

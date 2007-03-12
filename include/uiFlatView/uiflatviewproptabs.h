#ifndef uiflatviewproptabs_h
#define uiflatviewproptabs_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id: uiflatviewproptabs.h,v 1.3 2007-03-12 18:44:10 cvsbert Exp $
________________________________________________________________________

-*/

#include "colortab.h"
#include "flatview.h"
#include "uidlggroup.h"

class uiLabel;
class uiGenInput;
class uiCheckBox;
class uiColorInput;
class ColorTableEditor;
class uiLabeledComboBox;

    
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
    FlatView::Appearance& app_;

};
    
/*!\brief flat viewer properties tabs */

class uiFlatViewDataDispPropTab : public uiFlatViewPropTab
{
public:

    void		setDataNames(const FlatView::Data&);
    virtual void	setData(const FlatView::Data&)		= 0;

protected:
    			uiFlatViewDataDispPropTab(uiParent*,
					  FlatView::Viewer&,const char*);

    FlatView::DataDispPars& ddpars_;
    virtual FlatView::DataDispPars::Common& commonPars()	= 0;
    bool		doDisp() const;
    virtual const char*	dataName() const			= 0;

    uiLabeledComboBox*	dispfld_;
    uiGenInput*		useclipfld_;
    uiGenInput*		clipratiofld_;
    uiGenInput*		rgfld_;
    uiGenInput*		blockyfld_;

    uiObject*		lastcommonfld_;

    void		dispSel(CallBacker*);
    void		clipSel(CallBacker*);
    virtual void	handleFieldDisplay(bool)	= 0;

    void		putCommonToScreen();
    void		getCommonFromScreen();
    void		doSetData(const FlatView::Data&,bool);

};

		     
class uiFVWVAPropTab : public uiFlatViewDataDispPropTab
{
public:
    			uiFVWVAPropTab(uiParent*,FlatView::Viewer&);

    virtual void	putToScreen();
    virtual void	getFromScreen();
    virtual void	setData( const FlatView::Data& fvd )
			{ doSetData(fvd,true); }

protected:

    FlatView::DataDispPars::WVA& pars_;
    virtual FlatView::DataDispPars::Common& commonPars() { return pars_; }
    virtual const char*	dataName() const;

    uiGenInput*		overlapfld_;
    uiGenInput*		midlinefld_;
    uiGenInput*		midvalfld_;
    uiColorInput*       wigcolsel_;
    uiColorInput*       midlcolsel_;
    uiColorInput*       leftcolsel_;
    uiColorInput*       rightcolsel_;

    virtual void	handleFieldDisplay(bool);
    void		dispSel(CallBacker*);
    void		midlineSel(CallBacker*);
};


class uiFVVDPropTab : public uiFlatViewDataDispPropTab
{
public:
    			uiFVVDPropTab(uiParent*,FlatView::Viewer&);

    virtual void	putToScreen();
    virtual void	getFromScreen();
    virtual void	setData( const FlatView::Data& fvd )
			{ doSetData(fvd,false); }

protected:

    FlatView::DataDispPars::VD&		pars_;
    ColorTable		ctab_;
    virtual FlatView::DataDispPars::Common& commonPars() { return pars_; }
    virtual const char*	dataName() const;

    ColorTableEditor*	coltabfld_;
    uiLabel*		coltablbl_;

    virtual void	handleFieldDisplay(bool);
    void		dispSel(CallBacker*);
};


class uiFVAnnotPropTab : public uiFlatViewPropTab
{
public:

    			uiFVAnnotPropTab(uiParent*,FlatView::Viewer&,
					 const BufferStringSet* annots);

    virtual void	putToScreen();
    virtual void	getFromScreen();

    int			getSelAnnot() const	{ return x1_->getSelAnnot(); }
    void		setSelAnnot( int i )	{ x1_->setSelAnnot( i ); }


protected:
    
    FlatView::Annotation& annot_;

    class AxesGroup : public uiGroup
    {
    public:
			AxesGroup(uiParent*,FlatView::Annotation::AxisData&,
				  const BufferStringSet* annots=0);

	void		putToScreen();
	void		getFromScreen();

	int		getSelAnnot() const;
	void		setSelAnnot(int);

    protected:

	FlatView::Annotation::AxisData&	ad_;

	uiCheckBox*	showannotfld_;
	uiCheckBox*	showgridlinesfld_;
	uiCheckBox*	reversedfld_;
	uiGenInput*	annotselfld_;

    };

    uiColorInput*       colfld_;
    AxesGroup*		x1_;
    AxesGroup*		x2_;

};


#endif

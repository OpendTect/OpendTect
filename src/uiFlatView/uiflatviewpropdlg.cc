/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.cc,v 1.4 2007-03-02 10:55:17 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewpropdlg.h"

#include "coltabedit.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"

using namespace FlatView;


uiFlatViewPropTab::uiFlatViewPropTab( uiParent* p, FlatView::Viewer& vwr,
				      const char* lbl )
    : uiDlgGroup(p,lbl)
    , vwr_(vwr)
    , ctxt_(vwr.context())
{
}


uiFlatViewDataDispPropTab::uiFlatViewDataDispPropTab( uiParent* p,
		FlatView::Viewer& vwr, const char* tablbl )
    : uiFlatViewPropTab(p,vwr,tablbl)
    , ddpars_(ctxt_.ddpars_)
{
    dispfld_ = new uiGenInput( this, "Display", BoolInpSpec(true) );
    dispfld_->valuechanged.notify(
	    		mCB(this,uiFlatViewDataDispPropTab,dispSel) );

    useclipfld_ = new uiGenInput( this, "Use clipping", BoolInpSpec(true) );
    useclipfld_->valuechanged.notify(
	    		mCB(this,uiFlatViewDataDispPropTab,clipSel) );
    useclipfld_->attach( alignedBelow, dispfld_ );

    clipratiofld_ = new uiGenInput( this, "Percentage clip", FloatInpSpec() );
    clipratiofld_->setElemSzPol(uiObject::Small);
    clipratiofld_->attach( alignedBelow, useclipfld_ );
    clipratiofld_->display( useclipfld_->getBoolValue() );

    rgfld_ = new uiGenInput( this, "Range", FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, useclipfld_ );
    rgfld_->display( !useclipfld_->getBoolValue() );
    
    blockyfld_ = new uiGenInput( this,
	    	 "Display blocky (no interpolation)", BoolInpSpec(true) );
    blockyfld_->attach( alignedBelow, rgfld_ );

    lastcommonfld_ = blockyfld_->attachObj();
}


bool uiFlatViewDataDispPropTab::doDisp() const
{
    return dispfld_->getBoolValue();
}


void uiFlatViewDataDispPropTab::clipSel(CallBacker*)
{
    const bool dodisp = doDisp();
    const bool useclip = useclipfld_->getBoolValue();
    clipratiofld_->display( dodisp && useclip );
    rgfld_->display( dodisp && !useclip );
}


void uiFlatViewDataDispPropTab::dispSel( CallBacker* )
{
    const bool dodisp = doDisp();
    useclipfld_->display( dodisp );
    blockyfld_->display( dodisp );
    clipSel( 0 );
    handleFieldDisplay( dodisp );
}


void uiFlatViewDataDispPropTab::putCommonToScreen()
{
    const DataDispPars::Common& pars = commonPars();
    dispfld_->setValue( pars.show_ );
    useclipfld_->setValue( mIsUdf( pars.rg_.start ) );
    clipratiofld_->setValue( pars.clipperc_ );
    rgfld_->setValue( pars.rg_ );
    blockyfld_->setValue( pars.blocky_ );
    dispSel( 0 );
}


void uiFlatViewDataDispPropTab::getCommonFromScreen()
{
    DataDispPars::Common& pars = commonPars();

    pars.show_ = doDisp();
    pars.clipperc_ = useclipfld_->getBoolValue()
				? clipratiofld_->getfValue() : 0;
    pars.rg_ = useclipfld_->getBoolValue()
			    ? Interval<float>(mUdf(float),mUdf(float))
			    : rgfld_->getFInterval();
    pars.blocky_ = blockyfld_->getBoolValue();
}


uiFVWVAPropTab::uiFVWVAPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,"Wiggle Variable Area")
    , pars_(ddpars_.wva_)
{
    overlapfld_ = new uiGenInput( this, "Overlap ratio", FloatInpSpec() );
    overlapfld_->setElemSzPol(uiObject::Small);
    overlapfld_->attach( alignedBelow, lastcommonfld_ );

    leftcolsel_ = new uiColorInput( this, pars_.left_, "Left fill", true );
    leftcolsel_->attach( alignedBelow, overlapfld_ );
    
    rightcolsel_ = new uiColorInput( this, pars_.right_, "Right fill", true);
    rightcolsel_->attach( rightTo, leftcolsel_ );

    wigcolsel_ = new uiColorInput( this, pars_.wigg_, "Draw wiggles", true );
    wigcolsel_->attach( alignedBelow, leftcolsel_ );
    
    midlcolsel_ = new uiColorInput( this, pars_.mid_, "Middle line", true );
    midlcolsel_->attach( rightOf, wigcolsel_ );
    rightcolsel_->attach( alignedWith, midlcolsel_ );
    midlcolsel_->dodrawchanged.notify( mCB(this,uiFVWVAPropTab,midlineSel) );

    midlinefld_ = new uiGenInput( this, "Display middle line at",
			BoolInpSpec(true,"Specified value","Median value"));
    midlinefld_->valuechanged.notify( mCB(this,uiFVWVAPropTab,midlineSel) );
    midlinefld_->attach( alignedBelow, wigcolsel_ );

    midvalfld_ = new uiGenInput( this, "Middle line value", FloatInpSpec() );
    midvalfld_->setElemSzPol(uiObject::Small);
    midvalfld_->attach( alignedBelow, midlinefld_ );
}


void uiFVWVAPropTab::handleFieldDisplay( bool dodisp )
{
    wigcolsel_->display( dodisp );
    midlcolsel_->display( dodisp );
    leftcolsel_->display( dodisp );
    rightcolsel_->display( dodisp );
    overlapfld_->display( dodisp );

    midlineSel( 0 );
}


void uiFVWVAPropTab::midlineSel(CallBacker*)
{
    const bool dodisp = doDisp();
    const bool havecol = midlcolsel_->doDraw();
    midvalfld_->display( dodisp && havecol && midlinefld_->getBoolValue() );
    midlinefld_->display( dodisp && havecol );
}


void uiFVWVAPropTab::putToScreen()
{
    overlapfld_->setValue( pars_.overlap_ );
    midlinefld_->setValue( !mIsUdf(pars_.midvalue_) );
    midvalfld_->setValue( pars_.midvalue_ );

#define mSetCol(fld,memb) \
    havecol = !(pars_.memb == Color::NoColor); \
    fld->setColor( havecol ? pars_.memb : Color::Black ); \
    fld->setDoDraw( havecol )

    bool mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);

#undef mSetCol

    putCommonToScreen();
}


void uiFVWVAPropTab::getFromScreen()
{
    getCommonFromScreen();
    if ( !pars_.show_ ) return;

    pars_.overlap_ = overlapfld_->getfValue();
    pars_.midvalue_ = midlinefld_->getBoolValue() ? midvalfld_->getfValue() 
						  : mUdf(float);
#define mSetCol(fld,memb) \
    pars_.memb = fld->doDraw() ? fld->color(): Color::NoColor
    mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);
#undef mSetCol
}


uiFVVDPropTab::uiFVVDPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,"Variable Density")
    , pars_(ddpars_.vd_)
    , ctab_( ddpars_.vd_.ctab_.buf() )
{
    coltabfld_ =  new ColorTableEditor( this,
		    ColorTableEditor::Setup().vertical(false).editable(false)
					     .withclip(false), &ctab_ );
    coltablbl_ = new uiLabel( this, "Color table", coltabfld_ );
    coltabfld_->attach( alignedBelow, lastcommonfld_ );
}


void uiFVVDPropTab::handleFieldDisplay( bool dodisp )
{
    coltabfld_->display( dodisp );
    coltablbl_->display( dodisp );
}


void uiFVVDPropTab::putToScreen()
{
    ColorTable::get( pars_.ctab_, ctab_ );
    coltabfld_->tableChanged( 0 );
    putCommonToScreen();
}


void uiFVVDPropTab::getFromScreen()
{
    getCommonFromScreen();
    if ( !pars_.show_ ) return;

    pars_.ctab_ = coltabfld_->getColorTable()->name();
}


uiFVAnnotPropTab::AxesGroup::AxesGroup( uiParent* p,
					FlatView::Annotation::AxisData& ad )
    : uiGroup(p,"Axis Data")
    , ad_(ad)
{
    namefld_ = new uiGenInput( this, "Axis name", ad_.name_ );
    reversedfld_ = new uiCheckBox( this, "Reversed" );
    reversedfld_->attach( rightOf, namefld_ );
    showannotfld_ = new uiCheckBox( this, "Annotation" );
    showannotfld_->attach( alignedBelow, namefld_ );
    uiLabel* lbl = new uiLabel( this, "Show" );
    lbl->attach( leftOf, showannotfld_ );
    showgridlinesfld_ = new uiCheckBox( this, "Grid lines" );
    showgridlinesfld_->attach( rightOf, showannotfld_ );

    setHAlignObj( namefld_ );
}


void uiFVAnnotPropTab::AxesGroup::putToScreen()
{
    namefld_->setText( ad_.name_ );
    reversedfld_->setChecked( ad_.reversed_ );
    showannotfld_->setChecked( ad_.showannot_ );
    showgridlinesfld_->setChecked( ad_.showgridlines_ );
}


void uiFVAnnotPropTab::AxesGroup::getFromScreen()
{
    ad_.name_ = namefld_->text();
    ad_.reversed_ = reversedfld_->isChecked();
    ad_.showannot_ = showannotfld_->isChecked();
    ad_.showgridlines_ = showgridlinesfld_->isChecked();
}


uiFVAnnotPropTab::uiFVAnnotPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewPropTab(p,vwr,"Annotation")
    , annot_(ctxt_.annot_)
{
    x1_ = new AxesGroup( this, annot_.x1_ );
    x2_ = new AxesGroup( this, annot_.x2_ );
    x2_->attach( alignedBelow, x1_ );
}


void uiFVAnnotPropTab::putToScreen()
{
    x1_->putToScreen();
    x2_->putToScreen();
}


void uiFVAnnotPropTab::getFromScreen()
{
    x1_->getFromScreen();
    x2_->getFromScreen();
}

		    
uiFlatViewPropDlg::uiFlatViewPropDlg( uiParent* p, FlatView::Viewer& vwr,
				      const CallBack& applcb )
    : uiTabStackDlg(p,uiDialog::Setup("Display properties",
				      "Specify display properties",
				      "51.0.0"))
    , applycb_(applcb)
{
    wvatab_ = new uiFVWVAPropTab( tabParent(), vwr );
    addGroup( wvatab_ );
    vdtab_ = new uiFVVDPropTab( tabParent(), vwr );
    addGroup( vdtab_ );
    annottab_ = new uiFVAnnotPropTab( tabParent(), vwr );
    addGroup( annottab_ );

    putAllToScreen();

    uiPushButton* applybut = new uiPushButton( this, "&Apply",
			     mCB(this,uiFlatViewPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton( "Save as Default" );
}


void uiFlatViewPropDlg::getAllFromScreen()
{
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab&,ptab,getGroup(idx))
	ptab.getFromScreen();
    }
}


void uiFlatViewPropDlg::putAllToScreen()
{
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab&,ptab,getGroup(idx))
	ptab.putToScreen();
    }
}


void uiFlatViewPropDlg::doApply( CallBacker* )
{
    getAllFromScreen();
    applycb_.doCall( this );
}

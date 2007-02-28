/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.cc,v 1.1 2007-02-28 15:58:44 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiflatviewpropdlg.h"

#include "coltabedit.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"

using namespace FlatView;

uiFlatViewPropTab::uiFlatViewPropTab( 	uiParent* p,
					const DataDispPars::Common& pars,
					const char* tablbl )
    : uiDlgGroup(p,tablbl)
{
    useclipfld_ = new uiGenInput( this, "Use clipping", BoolInpSpec(true) );
    useclipfld_->setValue( mIsUdf( pars.rg_.start ) );
    useclipfld_->valuechanged.notify( mCB(this,uiFlatViewPropTab,clipSel) );

    clipratiofld_ = new uiGenInput( this, "Percentage clip", FloatInpSpec() );
    clipratiofld_->setValue( pars.clipperc_ );
    clipratiofld_->setElemSzPol(uiObject::Small);
    clipratiofld_->attach( alignedBelow, useclipfld_ );
    clipratiofld_->display( useclipfld_->getBoolValue() );

    rgfld_ = new uiGenInput( this, "Range", FloatInpIntervalSpec() );
    rgfld_->setValue( pars.rg_ );
    rgfld_->attach( alignedBelow, useclipfld_ );
    rgfld_->display( !useclipfld_->getBoolValue() );
    
    blockyfld_ = new uiGenInput( this,
	    	 "Display blocky (no interpolation)", BoolInpSpec(true) );
    blockyfld_->setValue( pars.blocky_ );
    blockyfld_->attach( alignedBelow, rgfld_ );
    clipSel(0);
}


void uiFlatViewPropTab::clipSel(CallBacker*)
{
    bool yn = useclipfld_->getBoolValue();
    clipratiofld_->display( yn );
    rgfld_->display( !yn );
}


void uiFlatViewPropTab::fillDispPars( DataDispPars::Common& compars ) const
{
    compars.clipperc_ = useclipfld_->getBoolValue()
				? clipratiofld_->getfValue() : 0;
    compars.rg_ = useclipfld_->getBoolValue()
			    ? Interval<float>(mUdf(float),mUdf(float))
			    : rgfld_->getFInterval();
    compars.blocky_ = blockyfld_->getBoolValue();
}


uiWVAFVPropTab::uiWVAFVPropTab( uiParent* p, const DataDispPars::WVA& wvapars,
       			    bool dispwva )
    : uiFlatViewPropTab(p,wvapars,"Wiggle Variable Area")
    , wvapars_( wvapars )
    , dispwva_( dispwva )
{
    dispfld_ = new uiGenInput( this, "Display", BoolInpSpec(true) );
    dispfld_->valuechanged.notify( mCB(this,uiWVAFVPropTab,dispSel) );
    dispfld_->setValue( dispwva_ );
    
    useclipfld_->attach( alignedBelow, dispfld_ );

    overlapfld_ = new uiGenInput( this, "Overlap ratio", FloatInpSpec() );
    overlapfld_->setValue( wvapars_.overlap_ );
    overlapfld_->setElemSzPol(uiObject::Small);
    overlapfld_->attach( alignedBelow, blockyfld_ );

    leftcolsel_ = new uiColorInput( this, wvapars_.left_, "Left fill", true );
    leftcolsel_->attach( alignedBelow, overlapfld_ );
    
    rightcolsel_ = new uiColorInput( this, wvapars_.right_, "Right fill", true);
    rightcolsel_->attach( rightTo, leftcolsel_ );

    wigcolsel_ = new uiColorInput( this, wvapars_.wigg_, "Draw wiggles", true );
    wigcolsel_->attach( alignedBelow, leftcolsel_ );
    
    midlcolsel_ = new uiColorInput( this, wvapars_.mid_, "Middle line", true );
    midlcolsel_->attach( rightOf, wigcolsel_ );
    rightcolsel_->attach( alignedWith, midlcolsel_ );
    midlcolsel_->dodrawchanged.notify( mCB(this,uiWVAFVPropTab,midlineSel) );

    midlinefld_ = new uiGenInput( this, "Display middle line at",
			BoolInpSpec(true,"Specified value","Median value"));
    midlinefld_->valuechanged.notify( mCB(this,uiWVAFVPropTab,midlineSel) );
    midlinefld_->setValue( !mIsUdf(wvapars_.midvalue_) );
    midlinefld_->attach( alignedBelow, wigcolsel_ );

    midvalfld_ = new uiGenInput( this, "Middle line value", FloatInpSpec() );
    midvalfld_->setValue( wvapars_.midvalue_ );
    midvalfld_->setElemSzPol(uiObject::Small);
    midvalfld_->attach( alignedBelow, midlinefld_ );

    clipSel(0);
    dispSel(0);

    // Define this here because setting checkboxes will emit callbacks
    // Therefore, we must make sure all fields are created
    // TODO : make sure checkboxes do NOT emit callbacks when set from code
#define mSetCol(fld,memb) \
    havecol = !(wvapars_.memb == Color::NoColor); \
    fld->setColor( havecol ? wvapars_.memb : Color::Black ); \
    fld->setDoDraw( havecol )

    bool mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);

#undef mSetCol
}


void uiWVAFVPropTab::dispSel(CallBacker*)
{
    const bool yn = dispfld_->getBoolValue();
    const bool useclip = useclipfld_->getBoolValue();
    useclipfld_->display(yn);
    clipratiofld_->display( yn && useclip );
    rgfld_->display( yn && !useclip );
    blockyfld_->display(yn);
    wigcolsel_->display(yn);
    midlcolsel_->display(yn);
    leftcolsel_->display(yn);
    rightcolsel_->display(yn);
    overlapfld_->display(yn);

    midlineSel( 0 );
}


void uiWVAFVPropTab::midlineSel(CallBacker*)
{
    const bool havecol = dispfld_->getBoolValue() && midlcolsel_->doDraw();
    midvalfld_->display( havecol && midlinefld_->getBoolValue() );
    midlinefld_->display( havecol );
}


bool uiWVAFVPropTab::acceptOK()
{
    fillDispPars();
    return true;
}


void uiWVAFVPropTab::fillDispPars()
{
    dispwva_ = dispfld_->getBoolValue();
    if ( !dispwva_ )
	return;
    
    uiFlatViewPropTab::fillDispPars( wvapars_ );
    wvapars_.overlap_ = overlapfld_->getfValue();
    wvapars_.midvalue_ = midlinefld_->getBoolValue() ? midvalfld_->getfValue() 
						     : mUdf(float);
#define mSetCol(fld,memb) \
    wvapars_.memb = fld->doDraw() ? fld->color(): Color::NoColor
    mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);
#undef mSetCol
}


uiVDFVPropTab::uiVDFVPropTab( uiParent* p, const DataDispPars::VD& vdpars,
			  bool dispvd )
    : uiFlatViewPropTab(p,vdpars,"Variable Density")
    , vdpars_( vdpars )
    , ctab_( vdpars_.ctab_.buf() )
    , dispvd_( dispvd )
{
    dispfld_ = new uiGenInput( this, "Display", BoolInpSpec(true) );
    dispfld_->valuechanged.notify( mCB(this,uiVDFVPropTab,dispSel) );
    dispfld_->setValue( dispvd_ );
    
    useclipfld_->attach( alignedBelow, dispfld_ );

    coltabfld_ =  new ColorTableEditor( this,
		    ColorTableEditor::Setup().vertical(false).editable(false)
					     .withclip(false), &ctab_ );
    coltabfld_->attach( alignedBelow, blockyfld_ );
    coltablbl_ = new uiLabel( this, "Color table", coltabfld_ );
    clipSel(0);
    dispSel(0);
}


void uiVDFVPropTab::clipSel(CallBacker*)
{
    uiFlatViewPropTab::clipSel(0);
    coltabfld_->tableChanged();
}


void uiVDFVPropTab::dispSel(CallBacker*)
{
    bool yn = dispfld_->getBoolValue();
    bool useclip = useclipfld_->getBoolValue();
    useclipfld_->display(yn);
    clipratiofld_->display( yn && useclip );
    rgfld_->display( yn && !useclip );
    blockyfld_->display(yn);
    coltabfld_->display(yn);
    coltablbl_->display(yn);
}


bool uiVDFVPropTab::acceptOK()
{
    fillDispPars();
    return true;
}


void uiVDFVPropTab::fillDispPars()
{
    dispvd_ = dispfld_->getBoolValue();
    if ( !dispvd_ )
	return;

    uiFlatViewPropTab::fillDispPars( vdpars_ );
    ctab_ = *coltabfld_->getColorTable();
    vdpars_.ctab_ = ctab_.name();
}


uiFlatViewPropDlg::uiFlatViewPropDlg( uiParent* p,
					const FlatView::DataDispPars& ddp,
					const CallBack& applcb )
    : uiTabStackDlg(p,uiDialog::Setup("Display properties",
				      "Specify display properties",
				      "51.0.0"))
{
    wvaproptab_ = new uiWVAFVPropTab( tabParent(), ddp.wva_, ddp.dispwva_ );
    addGroup( wvaproptab_ );
    vdproptab_ = new uiVDFVPropTab( tabParent(), ddp.vd_, ddp.dispvd_ );
    addGroup( vdproptab_ );

    uiPushButton* applybut = new uiPushButton( this, "&Apply", applcb, true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton( "Save as Default" );
}

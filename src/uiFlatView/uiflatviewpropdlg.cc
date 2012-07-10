/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Dec 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiflatviewpropdlg.cc,v 1.67 2012-07-10 13:27:27 cvsbruno Exp $";

#include "uiflatviewpropdlg.h"
#include "uiflatviewproptabs.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uiseparator.h"

#include "datapackbase.h"


uiFlatViewPropTab::uiFlatViewPropTab( uiParent* p, FlatView::Viewer& vwr,
				      const char* lbl )
    : uiDlgGroup(p,lbl)
    , vwr_(vwr)
    , app_(vwr.appearance())
    , dpm_(DPM(DataPackMgr::FlatID()))
{
}


uiFlatViewDataDispPropTab::uiFlatViewDataDispPropTab( uiParent* p,
		FlatView::Viewer& vwr, const char* tablbl, bool show )
    : uiFlatViewPropTab(p,vwr,tablbl)
    , ddpars_(app_.ddpars_)
    , showdisplayfield_( show )
    , blockyfld_( 0 )
    , dispfld_( 0 )
{
    uiLabeledComboBox* lcb = 0;
    if ( showdisplayfield_ )
    {
	lcb = new uiLabeledComboBox( this, "Display" );
	dispfld_ = lcb->box();
	dispfld_->selectionChanged.notify(
			    mCB(this,uiFlatViewDataDispPropTab,dispSel) );
    }

    const char* clipmodes[] = { "None", "Symmetrical", "Asymmetrical", 0 };

    useclipfld_ = new uiGenInput( this, "Use clipping",
	    			  StringListInpSpec( clipmodes ) );
    useclipfld_->valuechanged.notify(
	    		mCB(this,uiFlatViewDataDispPropTab,clipSel) );
    if ( showdisplayfield_ )
	useclipfld_->attach( alignedBelow, lcb );

    symclipratiofld_ = new uiGenInput( this,"Percentage clip",FloatInpSpec() );
    symclipratiofld_->setElemSzPol(uiObject::Small);
    symclipratiofld_->attach( alignedBelow, useclipfld_ );
    symclipratiofld_->display( useclipfld_->getIntValue()==1 );
    symclipratiofld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    usemidvalfld_ = new uiGenInput( this, "Use mid value", BoolInpSpec(true) );
    usemidvalfld_->attach( alignedBelow, symclipratiofld_ );
    usemidvalfld_->display( useclipfld_->getIntValue()==1 );
    usemidvalfld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,useMidValSel) );
    
    symmidvalfld_ = new uiGenInput( this, "Mid value", FloatInpSpec() );
    symmidvalfld_->setElemSzPol(uiObject::Small);
    symmidvalfld_->attach( alignedBelow, usemidvalfld_ );
    symmidvalfld_->display( useclipfld_->getIntValue()==1 && 
			    usemidvalfld_->getBoolValue() );
    symmidvalfld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    assymclipratiofld_ = new uiGenInput( this,"Percentage clip",
	    				  FloatInpIntervalSpec() );
    assymclipratiofld_->setElemSzPol(uiObject::Small);
    assymclipratiofld_->attach( alignedBelow, useclipfld_ );
    assymclipratiofld_->display( useclipfld_->getIntValue()==2 );
    assymclipratiofld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    rgfld_ = new uiGenInput( this, "Range", FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, useclipfld_ );
    rgfld_->display( !useclipfld_->getBoolValue() );
   
    if ( show )
    {
    	blockyfld_ = new uiGenInput( this,
		"Display blocky (no interpolation)", BoolInpSpec(true) );
    	blockyfld_->attach( alignedBelow, symmidvalfld_ );
    }

    lastcommonfld_ = blockyfld_ ? blockyfld_->attachObj() : 0;

    /*
    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr)
    if ( uivwr )
	uivwr->dispParsChanged.notify( 
	    mCB(this,uiFlatViewDataDispPropTab,dispParsChanged ) );
	    */
}


uiFlatViewDataDispPropTab::~uiFlatViewDataDispPropTab()
{
    /*
    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_)
    if ( uivwr )
	uivwr->dispParsChanged.remove( 
	    mCB(this,uiFlatViewDataDispPropTab,dispParsChanged ) );
	    */
}


void uiFlatViewDataDispPropTab::dispParsChanged( CallBacker* )
{
    putCommonToScreen();
}


void uiFlatViewDataDispPropTab::useMidValSel( CallBacker* )
{
    symmidvalfld_->display( useclipfld_->getIntValue()==1 && 
			    usemidvalfld_->getBoolValue() );
    commonPars().mappersetup_.symmidval_ = mUdf( float );
    symmidvalfld_->setValue( mUdf( float ) );
}


void uiFlatViewDataDispPropTab::updateNonclipRange( CallBacker* )
{
    const int clip = useclipfld_->getIntValue();
    if ( clip<0 && clip>2 )
	return;
  
    FlatView::DataDispPars::Common& pars = commonPars();
    pars.mappersetup_.type_ = !clip ? ColTab::MapperSetup::Fixed
				    : ColTab::MapperSetup::Auto;

    if ( clip == 0 )
    {
	mDynamicCastGet(const uiFVWVAPropTab*,wvatab,this)
	const bool wva = wvatab ? true : false;
	rgfld_->setValue( vwr_.getDataRange(wva) );
    }
    else if ( clip == 1 )
    {
	Interval<float> cliprate( symclipratiofld_->getfValue(),
				  symclipratiofld_->getfValue() );
	cliprate.start *= 0.01;
	cliprate.stop *= 0.01;
	pars.mappersetup_.cliprate_ = cliprate;
	pars.mappersetup_.symmidval_ = usemidvalfld_->getBoolValue() ?
	    symmidvalfld_->getfValue() : mUdf(float);
    }
    else if ( clip == 2 )
    {
	Interval<float> cliprate = assymclipratiofld_->getFInterval();
	cliprate.start *= 0.01;
	cliprate.stop *= 0.01;
	pars.mappersetup_.cliprate_ = cliprate;
	pars.mappersetup_.symmidval_ = mUdf(float);
	pars.mappersetup_.type_ = ColTab::MapperSetup::Auto;
    }

}


bool uiFlatViewDataDispPropTab::doDisp() const
{
    return showdisplayfield_ ? dispfld_->currentItem() != 0 : true;
}


void uiFlatViewDataDispPropTab::clipSel(CallBacker*)
{
    const bool dodisp = doDisp();
    const int clip = useclipfld_->getIntValue();
    symclipratiofld_->display( dodisp && clip==1 );
    symmidvalfld_->display( dodisp && clip==1 && 
	    		    usemidvalfld_->getBoolValue() );
    usemidvalfld_->display( dodisp && clip==1 );
    assymclipratiofld_->display( dodisp && clip==2 );
    rgfld_->display( dodisp && !clip );

    updateNonclipRange( 0 );
}


void uiFlatViewDataDispPropTab::dispSel( CallBacker* )
{
    const bool dodisp = doDisp();
    useclipfld_->display( dodisp );
    if ( blockyfld_ ) 
	blockyfld_->display( dodisp );

    clipSel( 0 );
    handleFieldDisplay( dodisp );
}


void uiFlatViewDataDispPropTab::setDataNames()
{
    if ( !showdisplayfield_ )
	return;

    dispfld_->setEmpty();
    dispfld_->addItem( "No" );
    for ( int idx=0; idx<vwr_.availablePacks().size(); idx++ )
    {
	const DataPack* dp = dpm_.obtain( vwr_.availablePacks()[idx], true );
	if ( dp )
	{
	    dispfld_->addItem( dp->name() );
	    if ( dp->name() == dataName() )
		dispfld_->setCurrentItem( dispfld_->size() - 1 );
	}
    }
    mDynamicCastGet(const uiFVWVAPropTab*,wvatab,this)
    const bool wva = wvatab ? true : false;
    if ( (wva && !vwr_.appearance().ddpars_.wva_.show_) || 
	    (!wva && !vwr_.appearance().ddpars_.vd_.show_) )
	dispfld_->setCurrentItem( 0 );
}


void uiFlatViewDataDispPropTab::putCommonToScreen()
{
    const FlatView::DataDispPars::Common& pars = commonPars();
    bool havedata = pars.show_;
    if ( havedata && showdisplayfield_ )
    {
	const char* nm = dataName();
	if ( dispfld_->isPresent( nm ) )
	    dispfld_->setText( dataName() );
	else
	    havedata = false;
    }

    if ( !havedata && showdisplayfield_ )
	dispfld_->setCurrentItem( 0 );

    Interval<float> cliprate = pars.mappersetup_.cliprate_;
    if ( pars.mappersetup_.type_ == ColTab::MapperSetup::Fixed )
	useclipfld_->setValue( 0 );
    else if ( cliprate.width() == 0 )
	useclipfld_->setValue( 1 );
    else
	useclipfld_->setValue( 2 );

    mDynamicCastGet(const uiFVWVAPropTab*,wvatab,this)
    const bool wva = wvatab ? true : false;
    if ( !useclipfld_->getIntValue() )
	rgfld_->setValue( vwr_.getDataRange(wva) );

    symclipratiofld_->setValue( cliprate.start * 100 );
    Interval<float> assymclipperc( cliprate.start*100, cliprate.stop*100 );
    assymclipratiofld_->setValue( assymclipperc );

    const bool show = doDisp() && useclipfld_->getIntValue()==1 && 
		      mIsUdf(pars.mappersetup_.symmidval_);
    symmidvalfld_->display( show ); 
    symmidvalfld_->setValue( pars.mappersetup_.symmidval_ );
    usemidvalfld_->setValue( !mIsUdf(pars.mappersetup_.symmidval_) );
    usemidvalfld_->display( pars.mappersetup_.symmidval_ );
    if ( blockyfld_ )
	blockyfld_->setValue( pars.blocky_ );

    setDataNames();
    dispSel( 0 );
}


void uiFlatViewDataDispPropTab::doSetData( bool wva )
{
    if ( !showdisplayfield_ )
	return;

    if ( dispfld_->currentItem() == 0 )
	{ vwr_.usePack( wva, DataPack::cNoID(), false ); return; }

    const BufferString datanm( dispfld_->text() );
    for ( int idx=0; idx<vwr_.availablePacks().size(); idx++ )
    {
	const DataPack* dp = dpm_.obtain( vwr_.availablePacks()[idx], true );
	if ( dp && dp->name() == datanm )
	    vwr_.usePack( wva, dp->id(), false );
    }
}


bool uiFlatViewDataDispPropTab::acceptOK()
{
    FlatView::DataDispPars::Common& pars = commonPars();

    pars.show_ = doDisp();
    const int clip = useclipfld_->getIntValue();
    if ( clip != 0 )
	pars.mappersetup_.type_ = ColTab::MapperSetup::Auto;

    if ( !clip )
    {
	Interval<float> range = rgfld_->getFInterval();
	pars.mappersetup_.range_ = range;
    }
    else if ( clip==1 )
    {
	Interval<float> cliprate( symclipratiofld_->getfValue()*0.01,
				  symclipratiofld_->getfValue()*0.01 );
	pars.mappersetup_.cliprate_ = cliprate;
	pars.mappersetup_.symmidval_ = usemidvalfld_->getBoolValue() ?
				symmidvalfld_->getfValue() : mUdf(float);
    }
    else
    {
	Interval<float> cliprate = assymclipratiofld_->getFInterval();
	cliprate.start *= 0.01;
	cliprate.stop *= 0.01;
	pars.mappersetup_.cliprate_ = cliprate;
    }

    pars.blocky_ = blockyfld_ ? blockyfld_->getBoolValue() : false;
    setData();

    return true;
}


uiFVWVAPropTab::uiFVWVAPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,"Wiggle Variable Area")
    , pars_(ddpars_.wva_)
{
    overlapfld_ = new uiGenInput( this, "Overlap ratio", FloatInpSpec() );
    overlapfld_->setElemSzPol(uiObject::Small);
    overlapfld_->attach( alignedBelow, lastcommonfld_ );

    leftcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.left_).
			lbltxt("Left fill").withcheck(true).withdesc(false),
			"Left fill color" );
    leftcolsel_->attach( alignedBelow, overlapfld_ );
    
    rightcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.right_).
			 lbltxt("Right fill").withcheck(true).withdesc(false),
			 "Right fill color" );
    rightcolsel_->attach( rightTo, leftcolsel_ );

     wigcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.wigg_).
			lbltxt("Draw Wiggles").withcheck(true).withdesc(false),
			"Draw wiggles color" );

    wigcolsel_->attach( alignedBelow, leftcolsel_ );
    
    midlcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.mid_).
			lbltxt("Middle line").withcheck(true).withdesc(false),
			"Middle line color" );

    midlcolsel_->attach( rightOf, wigcolsel_ );
    rightcolsel_->attach( alignedWith, midlcolsel_ );
    midlcolsel_->doDrawChanged.notify( mCB(this,uiFVWVAPropTab,midlineSel) );

    midlinefld_ = new uiGenInput( this, "Display middle line at",
			BoolInpSpec(true,"Specified value","Median value") );
    midlinefld_->valuechanged.notify( mCB(this,uiFVWVAPropTab,midlineSel) );
    midlinefld_->attach( alignedBelow, wigcolsel_ );

    midvalfld_ = new uiGenInput( this, "Middle line value", FloatInpSpec() );
    midvalfld_->setElemSzPol(uiObject::Small);
    midvalfld_->attach( alignedBelow, midlinefld_ );
}


const char* uiFVWVAPropTab::dataName() const
{
    return vwr_.pack(true) ? vwr_.pack(true)->name().buf() : "";
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
    midlinefld_->setValue( !mIsUdf(pars_.midlinevalue_) );
    midvalfld_->setValue( pars_.midlinevalue_ );

#define mSetCol(fld,memb) \
    havecol = pars_.memb.isVisible(); \
    fld->setColor( havecol ? pars_.memb : Color::Black() ); \
    fld->setDoDraw( havecol )

    bool mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);

#undef mSetCol

    putCommonToScreen();
    const FlatView::DataDispPars::Common& pars = commonPars();
}


bool uiFVWVAPropTab::acceptOK()
{
    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    if ( !pars_.show_ ) return true;

    pars_.overlap_ = overlapfld_->getfValue();
    pars_.midlinevalue_ = midlinefld_->getBoolValue() ? midvalfld_->getfValue() 
						      : mUdf(float);
#define mSetCol(fld,memb) \
    pars_.memb = fld->doDraw() ? fld->color(): Color::NoColor()
    mSetCol(leftcolsel_,left_);
    mSetCol(rightcolsel_,right_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(midlcolsel_,mid_);
#undef mSetCol

    return true;
}


uiFVVDPropTab::uiFVVDPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,"Variable Density")
    , pars_(ddpars_.vd_)
    , ctab_( ddpars_.vd_.ctab_.buf() )
{
    uicoltab_ = new uiColorTable( this, ctab_.name().buf(), false );
    uicoltab_->setStretch( 0, 0 );
    uicoltablbl_ = new uiLabel( this, "Color table", uicoltab_ );
    uicoltab_->attach( alignedBelow, lastcommonfld_ );
}


const char* uiFVVDPropTab::dataName() const
{
    return vwr_.pack(false) ? vwr_.pack(false)->name().buf() : "";
}


void uiFVVDPropTab::handleFieldDisplay( bool dodisp )
{
    uicoltab_->display( dodisp );
    uicoltablbl_->display( dodisp );
}


void uiFVVDPropTab::putToScreen()
{
    ColTab::SM().get( pars_.ctab_, ctab_ );
    uicoltab_->setSequence( &ctab_, true );
    putCommonToScreen();
    const FlatView::DataDispPars::Common& pars = commonPars();
    Interval<float> range = pars.mappersetup_.range_;
    const bool udfrg = mIsUdf(range.start) && mIsUdf(range.stop);
    rgfld_->setValue( udfrg ? vwr_.getDataRange(false) : range );
}


bool uiFVVDPropTab::acceptOK()
{
    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    if ( pars_.show_ )
	pars_.ctab_ = uicoltab_->colTabSeq().name();

    return true;
}


uiFVAnnotPropTab::AxesGroup::AxesGroup( uiParent* p,
					FlatView::Annotation::AxisData& ad,
       					const BufferStringSet* annotnms,
       					bool dorevertaxis )
    : uiGroup(p,"Axis Data")
    , ad_(ad)
    , annotselfld_(0)
    , reversedfld_(0)		     
{
    BufferString lbltxt( "Axis '" ); lbltxt += ad_.name_; lbltxt += "'";
    uiLabel* lbl;
    const bool haveannotchoices = annotnms && annotnms->size() > 1;
    if ( !haveannotchoices )
	lbl = new uiLabel( this, lbltxt );
    else
    {
	annotselfld_ = new uiGenInput( this, lbltxt,
				       StringListInpSpec(*annotnms) );
	lbl = new uiLabel( this, "Show" );
    }
    showannotfld_ = new uiCheckBox( this, "Annotation" );
    if ( !haveannotchoices )
	showannotfld_->attach( rightOf, lbl );
    else
    {
	showannotfld_->attach( alignedBelow, annotselfld_ );
	lbl->attach( leftOf, showannotfld_ );
    }

    showgridlinesfld_ = new uiCheckBox( this, "Grid lines" );
    showgridlinesfld_->attach( rightOf, showannotfld_ );

    if ( dorevertaxis )
    {
	reversedfld_ = new uiCheckBox( this, "Reversed" );
	reversedfld_->attach( rightOf, showgridlinesfld_ );
    }

    setHAlignObj( showannotfld_ );
}


void uiFVAnnotPropTab::AxesGroup::putToScreen()
{
    if ( reversedfld_ )
	reversedfld_->setChecked( ad_.reversed_ );
    showannotfld_->setChecked( ad_.showannot_ );
    showgridlinesfld_->setChecked( ad_.showgridlines_ );
}


void uiFVAnnotPropTab::AxesGroup::getFromScreen()
{
    if ( reversedfld_ )
	ad_.reversed_ = reversedfld_->isChecked();
    ad_.showannot_ = showannotfld_->isChecked();
    ad_.showgridlines_ = showgridlinesfld_->isChecked();
}


int uiFVAnnotPropTab::AxesGroup::getSelAnnot() const
{
    if ( annotselfld_ )
    {
	const int res = annotselfld_->getIntValue();
	return res==-1 ? mUdf(int) : res;
    }

    return mUdf(int);
}


void uiFVAnnotPropTab::AxesGroup::setSelAnnot( int selannot )
{
    if ( annotselfld_ )
	annotselfld_->setValue( mIsUdf(selannot) ? -1 : selannot );
}


uiFVAnnotPropTab::uiFVAnnotPropTab( uiParent* p, FlatView::Viewer& vwr,
       				    const BufferStringSet* annots )
    : uiFlatViewPropTab(p,vwr,"Annotation")
    , annot_(app_.annot_)
    , auxnamefld_( 0 )
    , currentaux_( 0 )
{
    colfld_ = new uiColorInput( this, uiColorInput::Setup(annot_.color_).
	    			lbltxt("Annotation color"), 
				"Annotation color" );
    x1_ = new AxesGroup( this, annot_.x1_, annots, 
				annot_.allowuserchangereversedaxis_ );
    x1_->attach( alignedBelow, colfld_ );
    x2_ = new AxesGroup( this, annot_.x2_, 0,
				annot_.allowuserchangereversedaxis_ );
    x2_->attach( alignedBelow, x1_ );

    BufferStringSet auxnames;
    for ( int idx=0; idx<vwr_.nrAuxData(); idx++ )
    {
	const FlatView::AuxData& auxdata = *vwr_.getAuxData(idx);
	if ( auxdata.name_.isEmpty() || !auxdata.editpermissions_ )
	    continue;

	if ( !auxdata.enabled_ && !auxdata.editpermissions_->onoff_ )
	    continue;

	permissions_ += auxdata.editpermissions_;
	enabled_ += auxdata.enabled_;
	linestyles_ += auxdata.linestyle_;
	indices_ += idx;
	fillcolors_ += auxdata.fillcolor_;
	markerstyles_ += auxdata.markerstyles_.size()
	    ? auxdata.markerstyles_[0]
	    : MarkerStyle2D();
	x1rgs_ += auxdata.x1rg_ ? *auxdata.x1rg_ : Interval<double>( 0, 1 );
	x2rgs_ += auxdata.x2rg_ ? *auxdata.x2rg_ : Interval<double>( 0, 1 );
	auxnames.add( auxdata.name_.buf() );
    }

    if ( !auxnames.size() )
	return;

    auxnamefld_ = new uiGenInput( this, "Aux data",
	    			  StringListInpSpec( auxnames ) );
    auxnamefld_->valuechanged.notify( mCB( this, uiFVAnnotPropTab, auxNmFldCB));
    auxnamefld_->attach( alignedBelow, x2_ );

    linestylefld_ = new uiSelLineStyle( this, linestyles_[0], "Line style" );
    linestylefld_->attach( alignedBelow, auxnamefld_ );

    uiSelLineStyle::Setup su( "Line style" ); su.color( false );
    linestylenocolorfld_ = new uiSelLineStyle( this, linestyles_[0], su );
    linestylenocolorfld_->attach( alignedBelow, auxnamefld_ );

    fillcolorfld_ = new uiColorInput( this, uiColorInput::Setup(fillcolors_[0]),
				      "Fill color" );

    fillcolorfld_->attach( alignedBelow, linestylefld_ );

    x1rgfld_ = new uiGenInput( this, "X-Range", FloatInpIntervalSpec() );
    x1rgfld_->attach( alignedBelow, fillcolorfld_ );

    x2rgfld_ = new uiGenInput( this, "Y-Range", FloatInpIntervalSpec() );
    x2rgfld_->attach( alignedBelow, x1rgfld_ );
}


void uiFVAnnotPropTab::putToScreen()
{
    colfld_->setColor( annot_.color_ );
    x1_->putToScreen();
    x2_->putToScreen();

    for ( int idx=0; idx<indices_.size(); idx++ )
    {
	*permissions_[idx] = *vwr_.getAuxData(indices_[idx])->editpermissions_;
	enabled_[idx] = vwr_.getAuxData(indices_[idx])->enabled_;
	linestyles_[idx] = vwr_.getAuxData(indices_[idx])->linestyle_;
	fillcolors_[idx] = vwr_.getAuxData(indices_[idx])->fillcolor_;
	markerstyles_[idx] = vwr_.getAuxData(indices_[idx])->markerstyles_.size()
	    ? vwr_.getAuxData(indices_[idx])->markerstyles_[0]
	    : MarkerStyle2D();
	x1rgs_[idx] = vwr_.getAuxData(indices_[idx])->x1rg_
	    ? *vwr_.getAuxData(indices_[idx])->x1rg_
	    : Interval<double>( 0, 1 );
	x2rgs_[idx] = vwr_.getAuxData(indices_[idx])->x2rg_
	    ? *vwr_.getAuxData(indices_[idx])->x2rg_
	    : Interval<double>( 0, 1 );
    }

    if ( auxnamefld_ )
	updateAuxFlds( currentaux_ );
}


bool uiFVAnnotPropTab::acceptOK()
{
    annot_.color_ = colfld_->color();
    x1_->getFromScreen();
    x2_->getFromScreen();

    if ( !auxnamefld_ )
	return true;

    getFromAuxFld( currentaux_ );

    for ( int idx=0; idx<indices_.size(); idx++ )
    {
	vwr_.getAuxData(indices_[idx])->linestyle_ = linestyles_[idx];
	vwr_.getAuxData(indices_[idx])->fillcolor_ = fillcolors_[idx];
	if ( vwr_.getAuxData(indices_[idx])->markerstyles_.size() )
	    vwr_.getAuxData(indices_[idx])->markerstyles_[0]=markerstyles_[idx];
	vwr_.getAuxData(indices_[idx])->enabled_ = enabled_[idx];
	if ( vwr_.getAuxData(indices_[idx])->x1rg_ )
	    *vwr_.getAuxData(indices_[idx])->x1rg_ = x1rgs_[idx];
	if ( vwr_.getAuxData(indices_[idx])->x2rg_ )
	    *vwr_.getAuxData(indices_[idx])->x2rg_ = x2rgs_[idx];
    }

    return true;
}

		    
void uiFVAnnotPropTab::auxNmFldCB( CallBacker* )
{
    getFromAuxFld( currentaux_ );
    currentaux_ = auxnamefld_->getIntValue();
    updateAuxFlds( currentaux_ );
}


void uiFVAnnotPropTab::getFromAuxFld( int idx )
{
    if ( permissions_[idx]->linestyle_ && permissions_[idx]->linecolor_ )
	linestyles_[idx] = linestylefld_->getStyle();
    else if ( permissions_[idx]->linestyle_ && !permissions_[idx]->linecolor_ )
	linestyles_[idx].color_ = linestylenocolorfld_->getStyle().color_;

    if ( permissions_[idx]->fillcolor_ )
	fillcolors_[idx] = fillcolorfld_->color();

    if ( permissions_[idx]->x1rg_ )
	x1rgs_[idx] = x1rgfld_->getDInterval();

    if ( permissions_[idx]->x2rg_ )
	x2rgs_[idx] = x2rgfld_->getDInterval();
}


void uiFVAnnotPropTab::updateAuxFlds( int idx )
{
    if ( permissions_[idx]->linestyle_ && permissions_[idx]->linecolor_ )
    {
	linestylefld_->setStyle( linestyles_[idx] );
	linestylefld_->display( true );
	linestylenocolorfld_->display( false );
    }
    else if ( permissions_[idx]->linestyle_ && !permissions_[idx]->linecolor_ )
    {
	linestylenocolorfld_->setStyle( linestyles_[idx] );
	linestylenocolorfld_->display( true );
	linestylefld_->display( false );
    }

    if ( permissions_[idx]->fillcolor_ &&
	 vwr_.getAuxData(indices_[idx])->close_)
    {
	fillcolorfld_->setColor( fillcolors_[idx] );
	fillcolorfld_->display( true );
    }
    else
	fillcolorfld_->display( false );

    if ( permissions_[idx]->x1rg_ && vwr_.getAuxData(indices_[idx])->x1rg_ )
    {
	x1rgfld_->setValue( x1rgs_[idx] );
	x1rgfld_->display( true );
    }
    else
	x1rgfld_->display( false );

    if ( permissions_[idx]->x2rg_ && vwr_.getAuxData(indices_[idx])->x2rg_ )
    {
	x2rgfld_->setValue( x2rgs_[idx] );
	x2rgfld_->display( true );
    }
    else
	x2rgfld_->display( false );
}


uiFlatViewPropDlg::uiFlatViewPropDlg( uiParent* p, FlatView::Viewer& vwr,
				      const CallBack& applcb,
				      const BufferStringSet* annots,
       				      int selannot )
    : uiTabStackDlg(p,uiDialog::Setup("Display properties",
				      "Specify display properties",
				      "51.0.1"))
    , vwr_(vwr)
    , applycb_(applcb)
    , selannot_(selannot)
    , wvatab_(0)
    , annottab_(0)	
{
    vwr_.fillAppearancePar( initialpar_ );

    const bool wva = vwr_.appearance().ddpars_.wva_.allowuserchange_;
    const bool vd = vwr_.appearance().ddpars_.vd_.allowuserchange_;
    const bool annot = vwr_.appearance().annot_.allowuserchange_;

    if ( wva )
    {
	wvatab_ = new uiFVWVAPropTab( tabParent(), vwr_ );
	addGroup( wvatab_ );
    }

    if ( vd )
    {
	vdtab_ = new uiFVVDPropTab( tabParent(), vwr_ );
	addGroup( vdtab_ );
    }

    if ( annot )
    {
	annottab_ = new uiFVAnnotPropTab( tabParent(), vwr_, annots );
	addGroup( annottab_ );
    }

    titlefld_ = new uiGenInput( this, "Title" );
    titlefld_->attach( centeredAbove, tabObject() );

    uiPushButton* applybut = new uiPushButton( this, "&Apply",
			     mCB(this,uiFlatViewPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton( "Save as Default" );

    putAllToScreen();

    if ( wva && vwr_.packID(true)==DataPack::cNoID() &&
		 vwr_.packID(false)!=DataPack::cNoID() )
    {
	showGroup( 1 );
    }
}


void uiFlatViewPropDlg::putAllToScreen()
{
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab*,ptab,&getGroup(idx))
	if ( ptab ) ptab->putToScreen();
    }

    titlefld_->setText( vwr_.appearance().annot_.title_ );
    if ( annottab_ )
	annottab_->setSelAnnot( selannot_ );
}


void uiFlatViewPropDlg::doApply( CallBacker* cb )
{
    acceptOK( cb );
    vwr_.fillAppearancePar( initialpar_ );
    applycb_.doCall( this );
}


bool uiFlatViewPropDlg::rejectOK( CallBacker* cb )
{
    vwr_.useAppearancePar( initialpar_ );
    return true;
}


bool uiFlatViewPropDlg::acceptOK( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewer&,uivwr,vwr_);
    if ( !&uivwr )
	return false;
    //NotifyStopper notifystop( uivwr.dispParsChanged );

    if ( (wvatab_ && !wvatab_->doDisp()) && (vdtab_ && !vdtab_->doDisp()) )
    {
	uiMSG().error( "No data selected for Wiggle or VD display." );
	return false;
    }
     
    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    vwr_.appearance().annot_.title_ = titlefld_->text();
    if ( annottab_ )
	selannot_ = annottab_->getSelAnnot();

    return true;
}

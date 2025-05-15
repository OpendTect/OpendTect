/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewpropdlg.h"

#include "uicolor.h"
#include "uicolortable.h"
#include "uiflatviewer.h"
#include "uiflatviewproptabs.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "uiseparator.h"

#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"


// uiFlatViewPropTab

uiFlatViewPropTab::uiFlatViewPropTab( uiParent* p, FlatView::Viewer& vwr,
				      const uiString& lbl )
    : uiDlgGroup(p,lbl)
    , vwr_(vwr)
    , app_(vwr.appearance())
    , dpm_(DPM(DataPackMgr::FlatID()))
{
}


uiFlatViewPropTab::~uiFlatViewPropTab()
{
}


// uiFlatViewDataDispPropTab

uiFlatViewDataDispPropTab::uiFlatViewDataDispPropTab( uiParent* p,
		FlatView::Viewer& vwr, const uiString& tablbl, bool show )
    : uiFlatViewPropTab(p,vwr,tablbl)
    , ddpars_(app_.ddpars_)
    , showdisplayfield_(show)
{
    uiLabeledComboBox* lcb = nullptr;
    if ( showdisplayfield_ )
    {
	lcb = new uiLabeledComboBox( this, uiStrings::sDisplay() );
	dispfld_ = lcb->box();
	dispfld_->setHSzPol( uiObject::MedVar );
	dispfld_->selectionChanged.notify(
			mCB(this,uiFlatViewDataDispPropTab,dispSel) );
    }

    const char* clipmodes[] = { "None", "Symmetrical", "Asymmetrical", 0 };

    useclipfld_ = new uiGenInput(this, tr("Use clipping"),
				 StringListInpSpec( clipmodes ) );
    useclipfld_->valueChanged.notify(
			mCB(this,uiFlatViewDataDispPropTab,clipSel) );
    if ( showdisplayfield_ )
	useclipfld_->attach( alignedBelow, lcb );

    symclipratiofld_ = new uiGenInput( this,
				       tr("Percentage clip"),FloatInpSpec() );
    symclipratiofld_->setElemSzPol(uiObject::Small);
    symclipratiofld_->attach( alignedBelow, useclipfld_ );
    symclipratiofld_->display( useclipfld_->getIntValue()==1 );
    symclipratiofld_->valueChanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    usemidvalfld_ = new uiGenInput(this,
				   tr("Specify mid value"),BoolInpSpec(true));
    usemidvalfld_->attach( alignedBelow, symclipratiofld_ );
    usemidvalfld_->display( useclipfld_->getIntValue()==1 );
    usemidvalfld_->valueChanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,useMidValSel) );

    symmidvalfld_ = new uiGenInput( this,
				    tr("Mid value"), FloatInpSpec() );
    symmidvalfld_->setElemSzPol(uiObject::Small);
    symmidvalfld_->attach( alignedBelow, usemidvalfld_ );
    symmidvalfld_->display( useclipfld_->getIntValue()==1 &&
			    usemidvalfld_->getBoolValue() );
    symmidvalfld_->valueChanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    assymclipratiofld_ = new uiGenInput( this,
				    tr("Percentage clip"),
				    FloatInpIntervalSpec() );
    assymclipratiofld_->setElemSzPol(uiObject::Small);
    assymclipratiofld_->attach( alignedBelow, useclipfld_ );
    assymclipratiofld_->display( useclipfld_->getIntValue()==2 );
    assymclipratiofld_->valueChanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    rgfld_ = new uiGenInput( this, tr("Range"), FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, useclipfld_ );
    rgfld_->display( !useclipfld_->getBoolValue() );

    blockyfld_ = new uiGenInput( this, tr("Display blocky (no interpolation)"),
				 BoolInpSpec(true) );
    blockyfld_->attach( alignedBelow, symmidvalfld_ );

    lastcommonfld_ = blockyfld_ ? blockyfld_->attachObj() : nullptr;
}


uiFlatViewDataDispPropTab::~uiFlatViewDataDispPropTab()
{
    detachAllNotifiers();
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
    if ( clip<0 || clip>2 )
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
	Interval<float> cliprate( symclipratiofld_->getFValue(),
				  symclipratiofld_->getFValue() );
	cliprate.start_ *= 0.01;
	cliprate.stop_ *= 0.01;
	pars.mappersetup_.cliprate_ = cliprate;
	pars.mappersetup_.symmidval_ = usemidvalfld_->getBoolValue() ?
	    symmidvalfld_->getFValue() : mUdf(float);
    }
    else if ( clip == 2 )
    {
	Interval<float> cliprate = assymclipratiofld_->getFInterval();
	cliprate.start_ *= 0.01;
	cliprate.stop_ *= 0.01;
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


void uiFlatViewDataDispPropTab::setDataName( bool wva )
{
    if ( vwr_.hasPack(wva) )
    {
	auto dp = vwr_.getPack( wva ).get();
	if ( dp )
	{
	    if ( !dispfld_->isPresent(dp->name()) )
	    {
		dispfld_->addItem( toUiString(dp->name()) );
		if ( dp->name() == dataName() )
		    dispfld_->setCurrentItem( dispfld_->size() - 1 );
	    }
	}
    }
}


void uiFlatViewDataDispPropTab::setDataNames()
{
    if ( !showdisplayfield_ )
	return;

    dispfld_->setEmpty();
    dispfld_->addItem( uiStrings::sNone() );
    setDataName( true );
    setDataName( false );
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
	const BufferString nm = dataName();
	if ( dispfld_->isPresent( nm ) )
	    dispfld_->setText( nm );
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

    symclipratiofld_->setValue( cliprate.start_ * 100 );
    Interval<float> assymclipperc( cliprate.start_*100, cliprate.stop_*100 );
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


void uiFlatViewDataDispPropTab::fillCommonPar( IOPar& par ) const
{
    uiFlatViewPropTab::fillCommonPar( par );
}


void uiFlatViewDataDispPropTab::doSetData( bool wva )
{
    if ( !showdisplayfield_ )
	return;

    const FlatView::Viewer::VwrDest dest = wva ? FlatView::Viewer::WVA
					       : FlatView::Viewer::VD;
    if ( dispfld_->currentItem() == 0 )
	{ vwr_.setVisible( dest, false ); return; }

    const BufferString datanm( dispfld_->text() );
    bool updated = false;
    const bool canchanged = vwr_.enableChange( false );

    auto dp_wva = vwr_.getPack( true ).get();
    auto dp_vd = vwr_.getPack( false ).get();
    if ( wva && dp_vd && dp_vd->name()==datanm )
	vwr_.setPack( dest, dp_vd.ptr(), false );

    if ( !wva && dp_wva && dp_wva->name()==datanm )
	vwr_.setPack( dest, dp_wva.ptr(), false );

    vwr_.enableChange( canchanged );
    if ( updated )
	vwr_.handleChange( sCast(od_uint32,FlatView::Viewer::BitmapData) );
}


bool uiFlatViewDataDispPropTab::acceptOK()
{
    FlatView::DataDispPars::Common& pars = commonPars();

    pars.show_ = doDisp();
    const int clip = useclipfld_->getIntValue();
    if ( clip != 0 )
    {
	pars.mappersetup_.type_ = ColTab::MapperSetup::Auto;
	pars.mappersetup_.range_ = Interval<float>::udf();
    }

    if ( !clip )
    {
	Interval<float> range = rgfld_->getFInterval();
	pars.mappersetup_.range_ = range;
    }
    else if ( clip==1 )
    {
	Interval<float> cliprate( symclipratiofld_->getFValue()*0.01f,
				  symclipratiofld_->getFValue()*0.01f );
	pars.mappersetup_.cliprate_ = cliprate;
	pars.mappersetup_.symmidval_ = usemidvalfld_->getBoolValue() ?
				symmidvalfld_->getFValue() : mUdf(float);
    }
    else
    {
	Interval<float> cliprate = assymclipratiofld_->getFInterval();
	cliprate.start_ *= 0.01;
	cliprate.stop_ *= 0.01;
	pars.mappersetup_.cliprate_ = cliprate;
    }

    pars.blocky_ = blockyfld_ ? blockyfld_->getBoolValue() : false;
    setData();

    return true;
}


// uiFVWVAPropTab

uiFVWVAPropTab::uiFVWVAPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,tr("Wiggle Variable Area"),
      vwr.appearance().ddpars_.wva_.allowuserchangedata_)
    , pars_(ddpars_.wva_)
{
    overlapfld_ = new uiGenInput( this, tr("Overlap ratio"), FloatInpSpec() );
    overlapfld_->setElemSzPol(uiObject::Small);
    overlapfld_->attach( alignedBelow, lastcommonfld_ );

    leftcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.lowfill_).
			lbltxt(tr("Negative fill"))
			.withcheck(true).withdesc(false),
			"Negative fill color" );
    leftcolsel_->attach( alignedBelow, overlapfld_ );

    rightcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.highfill_).
			lbltxt(tr("Positive fill"))
			.withcheck(true).withdesc(false),
			 "Positive fill color" );
    rightcolsel_->attach( rightTo, leftcolsel_ );

    wigcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.wigg_).
			lbltxt(tr("Draw Wiggles"))
			.withcheck(true).withdesc(false),
			"Draw wiggles color" );
    wigcolsel_->attach( alignedBelow, leftcolsel_ );

    reflcolsel_ = new uiColorInput( this, uiColorInput::Setup(pars_.refline_).
			lbltxt(tr("Ref line")).withcheck(true).withdesc(false),
			"Ref line color" );
    reflcolsel_->attach( alignedBelow, rightcolsel_ );
    reflcolsel_->attach( ensureRightOf, wigcolsel_ );
    reflcolsel_->doDrawChanged.notify( mCB(this,uiFVWVAPropTab,reflineSel) );

    reflinefld_ = new uiGenInput( this, tr("Display reference line at"),
			BoolInpSpec(true,tr("Specified value"),
				    tr("Median value")) );
    reflinefld_->valueChanged.notify( mCB(this,uiFVWVAPropTab,reflineSel) );
    reflinefld_->attach( alignedBelow, wigcolsel_ );

    refvalfld_ = new uiGenInput( this, tr("Reference line value"),
				 FloatInpSpec() );
    refvalfld_->setElemSzPol(uiObject::Small);
    refvalfld_->attach( alignedBelow, reflinefld_ );

    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_);
    if ( uivwr )
	mAttachCB( uivwr->dispParsChanged, uiFVWVAPropTab::dispChgCB );
}


uiFVWVAPropTab::~uiFVWVAPropTab()
{
    detachAllNotifiers();
}


BufferString uiFVWVAPropTab::dataName() const
{
    ConstRefMan<FlatDataPack> dp = vwr_.getPack( true ).get();
    return BufferString( dp ? dp->name().str() : "" );
}


void uiFVWVAPropTab::handleFieldDisplay( bool dodisp )
{
    wigcolsel_->display( dodisp );
    reflcolsel_->display( dodisp );
    leftcolsel_->display( dodisp );
    rightcolsel_->display( dodisp );
    overlapfld_->display( dodisp );
    reflineSel( 0 );
}


void uiFVWVAPropTab::reflineSel(CallBacker*)
{
    const bool dodisp = doDisp();
    const bool havecol = reflcolsel_->doDraw();
    refvalfld_->display( dodisp && havecol && reflinefld_->getBoolValue() );
    reflinefld_->display( dodisp && havecol );
}


void uiFVWVAPropTab::dispChgCB( CallBacker* )
{
    putToScreen();
}


void uiFVWVAPropTab::putToScreen()
{
    overlapfld_->setValue( pars_.overlap_ );
    reflinefld_->setValue( !mIsUdf(pars_.reflinevalue_) );
    refvalfld_->setValue( pars_.reflinevalue_ );

#define mSetCol(fld,memb) \
    havecol = pars_.memb.isVisible(); \
    fld->setColor( havecol ? pars_.memb : OD::Color::Black() ); \
    fld->setDoDraw( havecol )

    bool mSetCol(leftcolsel_,lowfill_);
    mSetCol(rightcolsel_,highfill_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(reflcolsel_,refline_);

#undef mSetCol

    putCommonToScreen();
}


void uiFVWVAPropTab::fillCommonPar( IOPar& par ) const
{
    uiFlatViewDataDispPropTab::fillCommonPar( par );
}


bool uiFVWVAPropTab::acceptOK()
{
    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    if ( !pars_.show_ ) return true;

    pars_.overlap_ = overlapfld_->getFValue();
    pars_.reflinevalue_ = reflinefld_->getBoolValue() ? refvalfld_->getFValue()
						      : mUdf(float);
#define mSetCol(fld,memb) \
    pars_.memb = fld->doDraw() ? fld->color(): OD::Color::NoColor()
    mSetCol(leftcolsel_,lowfill_);
    mSetCol(rightcolsel_,highfill_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(reflcolsel_,refline_);
#undef mSetCol

    return true;
}


// uiFVVDPropTab

uiFVVDPropTab::uiFVVDPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,tr("Variable Density"),
      vwr.appearance().ddpars_.vd_.allowuserchangedata_)
    , pars_(ddpars_.vd_)
    , ctab_(ddpars_.vd_.ctab_.buf())
{
    ColTab::Sequence seq( ctab_.name().buf() );
    uicoltab_ = new uiColorTableGroup( this, seq );
    uicoltab_->enableManage( false );
    uicoltab_->enableClippingDlg( false );
    uicoltab_->setStretch( 0, 0 );
    uicoltablbl_ = new uiLabel( this, uiStrings::sColorTable(), uicoltab_ );
    uicoltab_->attach( alignedBelow, lastcommonfld_ );

    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_);
    if ( uivwr )
	mAttachCB( uivwr->dispParsChanged, uiFVVDPropTab::dispChgCB );
}


uiFVVDPropTab::~uiFVVDPropTab()
{
    detachAllNotifiers();
}


BufferString uiFVVDPropTab::dataName() const
{
    ConstRefMan<FlatDataPack> dp = vwr_.getPack( false ).get();
    return BufferString( dp ? dp->name().str() : "" );
}


void uiFVVDPropTab::handleFieldDisplay( bool dodisp )
{
    uicoltab_->display( dodisp );
    uicoltablbl_->display( dodisp );
}


void uiFVVDPropTab::dispChgCB( CallBacker* )
{
    putToScreen();
}


void uiFVVDPropTab::putToScreen()
{
    ColTab::SM().get( pars_.ctab_, ctab_ );
    uicoltab_->setDispPars( pars_ );
    uicoltab_->setSequence( &ctab_, true );
    uicoltab_->setInterval( vwr_.getDataRange(false) );
    putCommonToScreen();
    const FlatView::DataDispPars::Common& pars = commonPars();
    Interval<float> range = pars.mappersetup_.range_;
    const bool udfrg = mIsUdf(range.start_) && mIsUdf(range.stop_);
    rgfld_->setValue( udfrg ? vwr_.getDataRange(false) : range );
}


void uiFVVDPropTab::fillCommonPar( IOPar& par ) const
{
    uiFlatViewDataDispPropTab::fillCommonPar( par );
}


bool uiFVVDPropTab::acceptOK()
{
    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    pars_.mappersetup_.flipseq_ = uicoltab_->colTabMapperSetup().flipseq_;
    if ( pars_.show_ )
	pars_.ctab_ = uicoltab_->colTabSeq().name();

    return true;
}


// uiFVAnnotPropTab::AxesGroup

uiFVAnnotPropTab::AxesGroup::AxesGroup( uiParent* p, OD::Orientation orient,
					FlatView::Annotation::AxisData& ad,
					const uiStringSet* annotnms,
					int selannotdim, bool dorevertaxis )
    : uiGroup(p,"Axis Data")
    , ad_(ad)
{
    uiString axisstr = orient==OD::Horizontal ? uiStrings::sHorizontal()
					      : uiStrings::sVertical();
    const bool haveannotchoices = annotnms && annotnms->size() > 1;
    if ( haveannotchoices )
    {
	const uiString lbltxt = toUiString( "On %1 axis, show" ).arg(axisstr);
	annotselfld_ = new uiGenInput( this, lbltxt,
				       StringListInpSpec(*annotnms) );
	if ( annotnms->validIdx(selannotdim) )
	    setSelAnnot( selannotdim );
    }

    const uiString lbltxt = tr("%1 axis").arg(axisstr);
    auto* lbl = new uiLabel( this, lbltxt );
    showannotfld_ = new uiCheckBox( this, tr("Annotation") );
    lbl->attach( leftOf, showannotfld_ );
    if ( annotselfld_ )
	showannotfld_->attach( alignedBelow, annotselfld_ );

    showgridlinesfld_ = new uiCheckBox( this, tr("Grid lines") );
    showgridlinesfld_->attach( rightOf, showannotfld_ );

    if ( dorevertaxis )
    {
	reversedfld_ = new uiCheckBox( this, uiStrings::sReversed() );
	reversedfld_->attach( rightOf, showgridlinesfld_ );
    }

    auxlblfld_ = new uiLabel( this, ad_.auxlabel_ );
    showauxannotfld_ =
	new uiCheckBox( this, tr("Annotation"),
			mCB(this,uiFVAnnotPropTab::AxesGroup,showAuxCheckedCB));
    showauxannotfld_->attach( alignedBelow, showannotfld_ );
    auxlblfld_->attach( leftTo, showauxannotfld_ );
    uiSelLineStyle::Setup su;
    su.width( false );
    auxlinestylefld_ = new uiSelLineStyle( this, ad_.auxlinestyle_, su );
    auxlinestylefld_->attach( alignedBelow, showauxannotfld_ );

    setHAlignObj( showannotfld_ );
}


uiFVAnnotPropTab::AxesGroup::~AxesGroup()
{
    detachAllNotifiers();
}


void uiFVAnnotPropTab::AxesGroup::showAuxLineCheckedCB( CallBacker* )
{
    auxlinestylefld_->setSensitive( showauxannotfld_->isChecked() );
}


void uiFVAnnotPropTab::AxesGroup::showAuxCheckedCB( CallBacker* )
{
    auxlinestylefld_->setSensitive( showauxannotfld_->isChecked() );
}


void uiFVAnnotPropTab::AxesGroup::putToScreen()
{
    if ( reversedfld_ )
	reversedfld_->setChecked( ad_.reversed_ );

    showannotfld_->setChecked( ad_.showannot_ );
    showgridlinesfld_->setChecked( ad_.showgridlines_ );
    const bool hasauxdata = !ad_.auxannot_.isEmpty();
    auxlblfld_->setSensitive( hasauxdata );
    showauxannotfld_->setSensitive( hasauxdata );
    auxlinestylefld_->setSensitive( hasauxdata );
    if ( hasauxdata )
    {
	showauxannotfld_->setChecked( ad_.showauxannot_ );
	auxlinestylefld_->setStyle( ad_.auxlinestyle_ );
    }
}


void uiFVAnnotPropTab::AxesGroup::getFromScreen()
{
    if ( reversedfld_ )
	ad_.reversed_ = reversedfld_->isChecked();

    ad_.showannot_ = showannotfld_->isChecked();
    ad_.showgridlines_ = showgridlinesfld_->isChecked();
    ad_.showauxannot_ = showauxannotfld_->isChecked();
    ad_.auxlinestyle_ = auxlinestylefld_->getStyle();
}


int uiFVAnnotPropTab::AxesGroup::getSelAnnot() const
{
    return annotselfld_ ? annotselfld_->getIntValue() : -1;
}


void uiFVAnnotPropTab::AxesGroup::setSelAnnot( int selannot )
{
    if ( annotselfld_ )
	annotselfld_->setValue( selannot );
}


// uiFVAnnotPropTab

uiFVAnnotPropTab::uiFVAnnotPropTab( uiParent* p, FlatView::Viewer& vwr,
				    const uiStringSet* annotsdim0,
				    const uiStringSet* annotsdim1,
				    int selannotdim0, int selannotdim1 )
    : uiFlatViewPropTab(p,vwr,tr("Annotation"))
    , annot_(app_.annot_)
{
    colfld_ = new uiColorInput( this, uiColorInput::Setup(annot_.color_).
					lbltxt(tr("Annotation color")),
				"Annotation color" );

    auto* sep1 = new uiSeparator( this, "Separator 1" );
    sep1->attach( stretchedBelow, colfld_ );

    x1_ = new AxesGroup( this, OD::Horizontal, annot_.x1_, annotsdim0,
			 selannotdim0, annot_.allowuserchangereversedaxis_ );
    x1_->attach( alignedBelow, colfld_ );
    x1_->attach( ensureBelow, sep1 );

    auto* sep2 = new uiSeparator( this, "Separator 2" );
    sep2->attach( stretchedBelow, x1_ );

    x2_ = new AxesGroup( this, OD::Vertical, annot_.x2_, annotsdim1,
			 selannotdim1, annot_.allowuserchangereversedaxis_ );
    x2_->attach( alignedBelow, x1_ );
    x2_->attach( ensureBelow, sep2 );

    auto* sep3 = new uiSeparator( this, "Separator 3" );
    sep3->attach( stretchedBelow, x2_ );

    if ( vwr_.zDomain(false) )
    {
	viewnrdeczfld_ = new uiGenInput( this, tr("Decimal places for Z Value"),
			       IntInpSpec(vwr_.nrZDec(),0,vwr_.nrZDec()+2,1) );
	viewnrdeczfld_->attach( alignedBelow, x2_ );
	viewnrdeczfld_->attach( ensureBelow, sep3 );
    }

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

    if ( !auxnames.isEmpty() )
    {
	auxnamefld_ = new uiGenInput( this, tr("Aux data"),
				      StringListInpSpec( auxnames ) );
	auxnamefld_->valueChanged.notify(
			mCB( this, uiFVAnnotPropTab, auxNmFldCB));
	if ( viewnrdeczfld_ )
	    auxnamefld_->attach( alignedBelow, viewnrdeczfld_ );
	else
	    auxnamefld_->attach( alignedBelow, x2_ );

	linestylefld_ = new uiSelLineStyle( this, linestyles_[0],
					    tr("Line style") );
	linestylefld_->attach( alignedBelow, auxnamefld_ );

	uiSelLineStyle::Setup su( tr("Line style") );
	su.color( false );
	linestylenocolorfld_ = new uiSelLineStyle( this, linestyles_[0], su );
	linestylenocolorfld_->attach( alignedBelow, auxnamefld_ );

	fillcolorfld_ = new uiColorInput( this,
					  uiColorInput::Setup(fillcolors_[0]),
					  "Fill color" );
	fillcolorfld_->attach( alignedBelow, linestylefld_ );

	x1rgfld_ = new uiGenInput( this, tr("X-Range"),
				   FloatInpIntervalSpec() );
	x1rgfld_->attach( alignedBelow, fillcolorfld_ );

	x2rgfld_ = new uiGenInput( this, tr("Y-Range"),
				   FloatInpIntervalSpec() );
	x2rgfld_->attach( alignedBelow, x1rgfld_ );
    }

    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_)
    if ( uivwr )
	mAttachCB( uivwr->annotChanged, uiFVAnnotPropTab::annotChgdCB );
}


uiFVAnnotPropTab::~uiFVAnnotPropTab()
{
    detachAllNotifiers();
}


int uiFVAnnotPropTab::getSelAnnot( bool dim0 ) const
{
    const AxesGroup& ag = dim0 ? *x1_ : *x2_;
    return ag.getSelAnnot();
}


void uiFVAnnotPropTab::setSelAnnot( int idx, bool dim0 )
{
    AxesGroup& ag = dim0 ? *x1_ : *x2_;
    ag.setSelAnnot( idx );
}


int uiFVAnnotPropTab::nrZDecimals() const
{
    return viewnrdeczfld_ ? viewnrdeczfld_->getIntValue() : mUdf(int);
}


void uiFVAnnotPropTab::fillPar( IOPar& iopar ) const
{
    if ( viewnrdeczfld_ )
    {
	const int nrzdec = nrZDecimals();
	iopar.set( FlatView::Viewer::sKeyViewZnrDec(), nrzdec );
    }
}


void uiFVAnnotPropTab::annotChgdCB( CallBacker* )
{
    putToScreen();
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
	markerstyles_[idx] =
	    vwr_.getAuxData(indices_[idx])->markerstyles_.size() ?
	    vwr_.getAuxData(indices_[idx])->markerstyles_[0] : MarkerStyle2D();
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


void uiFVAnnotPropTab::fillCommonPar( IOPar& par ) const
{
    uiFlatViewPropTab::fillCommonPar( par );
    if ( viewnrdeczfld_ )
    {
	const ZDomain::Info* zdom = vwr_.zDomain( true );
	if ( zdom )
	    par.set( zdom->sKeyNrDec(), nrZDecimals() );
    }
}


bool uiFVAnnotPropTab::acceptOK()
{
    annot_.color_ = colfld_->color();
    x1_->getFromScreen();
    x2_->getFromScreen();
    const ZDomain::Info* zdom = vwr_.zDomain( true );
    if ( viewnrdeczfld_ && zdom )
	getNonConst(*zdom).setPreferredNrDec( nrZDecimals() );

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


// uiFlatViewPropDlg

uiFlatViewPropDlg::uiFlatViewPropDlg( uiParent* p, FlatView::Viewer& vwr,
				      const CallBack& applcb,
				      const uiStringSet* annotsdim0,
				      const uiStringSet* annotsdim1,
				      int selannotdim0, int selannotdim1,
				      bool withdynamictitle )
    : uiTabStackDlg(p,Setup(tr("Specify Display Properties"),
			     mODHelpKey(mODViewer2DPropDlgHelpID)).modal(false))
    , vwr_(vwr)
    , applycb_(applcb)
    , selannotdim0_(selannotdim0)
    , selannotdim1_(selannotdim1)
{
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
	annottab_ = new uiFVAnnotPropTab( tabParent(), vwr_,
					  annotsdim0, annotsdim1,
					  selannotdim0_, selannotdim1_ );
	addGroup( annottab_ );
    }

    auto* grp = new uiGroup( this, "Tile group" );
    grp->attach( centeredAbove, tabObject() );
    titlefld_ = new uiGenInput( grp, tr("Title") );

    if ( withdynamictitle )
    {
	titleoptfld_ = new uiGenInput( grp, tr("Title"),
	    BoolInpSpec(false,tr("Custom"),tr("Automatic")) );
	mAttachCB( titleoptfld_->valueChanged, uiFlatViewPropDlg::titleChgCB );

	titlefld_->attach( rightTo, titleoptfld_ );
    }

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
			     mCB(this,uiFlatViewPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton();

    mAttachCB( postFinalize(), uiFlatViewPropDlg::finalizeCB );
}


uiFlatViewPropDlg::~uiFlatViewPropDlg()
{
    detachAllNotifiers();
}


void uiFlatViewPropDlg::finalizeCB( CallBacker* )
{
    putAllToScreen();
    const bool wva = vwr_.appearance().ddpars_.wva_.allowuserchange_;
    if ( wva && !vwr_.getPack(true).get() && !vwr_.getPack(false).get() )
    {
	showGroup( 1 );
    }
}


void uiFlatViewPropDlg::titleChgCB( CallBacker* )
{
    if ( titleoptfld_ )
	titlefld_->display( titleoptfld_->getBoolValue() );
}


void uiFlatViewPropDlg::putAllToScreen()
{
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab*,ptab,&getGroup(idx))
	if ( ptab )
	    ptab->putToScreen();
    }

    titlefld_->setText( vwr_.appearance().annot_.title_ );
    if ( titleoptfld_ )
	titleoptfld_->setValue( !vwr_.appearance().annot_.dynamictitle_ );

    if ( annottab_ )
    {
	annottab_->setSelAnnot( selannotdim0_, true );
	annottab_->setSelAnnot( selannotdim1_, false );
    }

    titleChgCB( nullptr );
}


void uiFlatViewPropDlg::saveCommonSettings()
{
    IOPar par;
    wvatab_->fillCommonPar( par );
    vdtab_->fillCommonPar( par );
    if ( annottab_ )
	annottab_->fillCommonPar( par );

    if ( par.isEmpty() )
	return;

    Settings::common().merge( par );
    Settings::common().write();
}


void uiFlatViewPropDlg::doApply( CallBacker* cb )
{
    acceptOK( cb );
    applycb_.doCall( this );
}


bool uiFlatViewPropDlg::rejectOK( CallBacker* )
{
    putAllToScreen();
    return true;
}


bool uiFlatViewPropDlg::acceptOK( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_)
    if ( !uivwr ) return false;

    NotifyStopper ns1( uivwr->dispParsChanged );
    NotifyStopper ns2( uivwr->annotChanged );

    if ( (wvatab_ && !wvatab_->doDisp()) && (vdtab_ && !vdtab_->doDisp()) )
    {
	uiMSG().error( tr("No data selected for Wiggle or VD display.") );
	return false;
    }

    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    vwr_.appearance().annot_.title_ = titlefld_->text();
    vwr_.appearance().annot_.dynamictitle_ =
			titleoptfld_ && !titleoptfld_->getBoolValue();

    if ( annottab_ )
    {
	selannotdim0_ = annottab_->getSelAnnot( true );
	selannotdim1_ = annottab_->getSelAnnot( false );
	vwr_.setAnnotChoiceByIdx( selannotdim0_, true );
	vwr_.setAnnotChoiceByIdx( selannotdim1_, false );
    }

    return true;
}


int uiFlatViewPropDlg::selectedAnnot( bool dim0 ) const
{
    return dim0 ? selannotdim0_ : selannotdim1_;
}


void uiFlatViewPropDlg::fillPar( IOPar& iopar) const
{
    if ( annottab_ )
	annottab_->fillPar( iopar );
}

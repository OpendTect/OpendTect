/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          Dec 2006
________________________________________________________________________

-*/

#include "uiflatviewpropdlg.h"
#include "uiflatviewproptabs.h"

#include "uicolor.h"
#include "uicolsequsemodesel.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uisellinest.h"
#include "uiseparator.h"
#include "uisellinest.h"
#include "uicolseqsel.h"

#include "uistrings.h"
#include "od_helpids.h"


uiFlatViewPropTab::uiFlatViewPropTab( uiParent* p, FlatView::Viewer& vwr,
				      uiString lbl )
    : uiDlgGroup(p,lbl)
    , vwr_(vwr)
    , app_(vwr.appearance())
    , dpm_(DPM(DataPackMgr::FlatID()))
{
}


uiFlatViewDataDispPropTab::uiFlatViewDataDispPropTab( uiParent* p,
		FlatView::Viewer& vwr, const uiString& tablbl, bool show )
    : uiFlatViewPropTab(p,vwr,tablbl)
    , ddpars_(app_.ddpars_)
    , showdisplayfield_( show )
    , blockyfld_( 0 )
    , dispfld_( 0 )
{
    uiLabeledComboBox* lcb = 0;
    if ( showdisplayfield_ )
    {
	lcb = new uiLabeledComboBox( this, uiStrings::sDisplay() );
	dispfld_ = lcb->box();
	dispfld_->selectionChanged.notify(
			    mCB(this,uiFlatViewDataDispPropTab,dispSel) );
    }

    const char* clipmodes[] = { "None", "Symmetrical", "Asymmetrical", 0 };

    useclipfld_ = new uiGenInput(this, tr("Use clipping"),
				 StringListInpSpec( clipmodes ) );
    useclipfld_->valuechanged.notify(
			mCB(this,uiFlatViewDataDispPropTab,clipSel) );
    if ( showdisplayfield_ )
	useclipfld_->attach( alignedBelow, lcb );

    symclipratiofld_ = new uiGenInput( this,
				       tr("Percentage clip"),FloatInpSpec() );
    symclipratiofld_->setElemSzPol(uiObject::Small);
    symclipratiofld_->attach( alignedBelow, useclipfld_ );
    symclipratiofld_->display( useclipfld_->getIntValue()==1 );
    symclipratiofld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    assymclipratiofld_ = new uiGenInput( this, tr("Percentage clip"),
				    FloatInpSpec(), FloatInpSpec() );
    assymclipratiofld_->setElemSzPol(uiObject::Small);
    assymclipratiofld_->attach( alignedBelow, useclipfld_ );
    assymclipratiofld_->display( useclipfld_->getIntValue()==2 );
    assymclipratiofld_->valuechanged.notify(
	    mCB(this,uiFlatViewDataDispPropTab,updateNonclipRange) );

    rgfld_ = new uiGenInput( this, uiStrings::sRange(),
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, useclipfld_ );
    rgfld_->display( !useclipfld_->getBoolValue() );

    blockyfld_ = new uiGenInput( this, tr("Display blocky (no interpolation)"),
				 BoolInpSpec(true) );
    blockyfld_->attach( alignedBelow, symclipratiofld_ );

    lastcommonfld_ = blockyfld_ ? blockyfld_->attachObj() : 0;
}


void uiFlatViewDataDispPropTab::updateNonclipRange( CallBacker* )
{
    const int clip = useclipfld_->getIntValue();
    if ( clip<0 || clip>2 )
	return;

    FlatView::DataDispPars::Common& pars = commonPars();
    ColTab::MapperSetup* msu = pars.mapper_->setup().clone();

    if ( clip == 0 )
    {
	mDynamicCastGet(const uiFVWVAPropTab*,wvatab,this)
	const bool wva = wvatab ? true : false;
	const Interval<float> datarg = vwr_.getDataRange( wva );
	rgfld_->setValue( datarg );
	msu->setFixedRange( datarg );
    }
    else
    {
	msu->setNotFixed();
	if ( clip == 1 )
	{
	    const float perc = symclipratiofld_->getFValue();
	    ColTab::ClipRatePair cliprate( perc, perc );
	    ColTab::convFromPerc( cliprate );
	    msu->setClipRate( cliprate );
	}
	else if ( clip == 2 )
	{
	    const float perc0 = assymclipratiofld_->getFValue( 0 );
	    const float perc1 = assymclipratiofld_->getFValue( 1 );
	    ColTab::ClipRatePair cliprate( perc0, perc1 );
	    ColTab::convFromPerc( cliprate );
	    msu->setClipRate( cliprate );
	}
    }

    pars.mapper_->setup() = *msu;
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
    dispfld_->addItem( uiStrings::sNone() );
    for ( int idx=0; idx<vwr_.availablePacks().size(); idx++ )
    {
	auto dp = dpm_.getDP( vwr_.availablePacks()[idx]);
	if ( dp )
	{
	    dispfld_->addItem( toUiString(dp->name()) );
	    if ( dp->hasName(dataName()) )
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
	const BufferString nm = dataName();
	if ( dispfld_->isPresent( nm ) )
	    dispfld_->setText( nm );
	else
	    havedata = false;
    }

    if ( !havedata && showdisplayfield_ )
	dispfld_->setCurrentItem( 0 );

    const ColTab::MapperSetup& msu = pars.mapper_->setup();
    ColTab::ClipRatePair cliprate = msu.clipRate();
    if ( msu.isFixed() )
	useclipfld_->setValue( 0 );
    else if ( cliprate.first() == cliprate.second() )
	useclipfld_->setValue( 1 );
    else
	useclipfld_->setValue( 2 );

    mDynamicCastGet(const uiFVWVAPropTab*,wvatab,this)
    const bool wva = wvatab ? true : false;
    if ( !useclipfld_->getIntValue() )
	rgfld_->setValue( vwr_.getDataRange(wva) );

    symclipratiofld_->setValue( cliprate.first() * 100.f );
    assymclipratiofld_->setValue( cliprate.first() * 100.f, 0 );
    assymclipratiofld_->setValue( cliprate.second() * 100.f, 1 );

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
    { vwr_.setVisible( wva, false ); return; }

    const BufferString datanm( dispfld_->text() );
    for ( int idx=0; idx<vwr_.availablePacks().size(); idx++ )
    {
	auto dp = dpm_.getDP( vwr_.availablePacks()[idx] );
	if ( dp && dp->hasName(datanm) )
	    vwr_.usePack( wva, dp->id(), false );
    }
}


bool uiFlatViewDataDispPropTab::acceptOK()
{
    FlatView::DataDispPars::Common& pars = commonPars();

    pars.show_ = doDisp();
    const int clip = useclipfld_->getIntValue();
    ColTab::MapperSetup* msu = pars.mapper_->setup().clone();

    if ( clip == 0 )
	msu->setFixedRange( rgfld_->getFInterval() );
    else if ( clip==1 )
    {
	msu->setNotFixed();
	float val = symclipratiofld_->getFValue();
	if ( mIsUdf(val) )
	    val = 0.f;
	else
	    val *= 0.01f;
	msu->setClipRate( ColTab::ClipRatePair(val,val) );
    }
    else
    {
	msu->setNotFixed();
	ColTab::ClipRatePair clips;
	clips.first() = assymclipratiofld_->getFValue(0) * 0.01f;
	clips.second() = assymclipratiofld_->getFValue(1) * 0.01f;
	msu->setClipRate( clips );
    }

    pars.mapper_->setup() = *msu;
    pars.blocky_ = blockyfld_ ? blockyfld_->getBoolValue() : false;
    setData();

    return true;
}


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
    reflinefld_->valuechanged.notify( mCB(this,uiFVWVAPropTab,reflineSel) );
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
    ConstRefMan<FlatDataPack> dp = vwr_.getPack(true);
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
    fld->setColor( havecol ? pars_.memb : Color::Black() ); \
    fld->setDoDraw( havecol )

    bool mSetCol(leftcolsel_,lowfill_);
    mSetCol(rightcolsel_,highfill_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(reflcolsel_,refline_);

#undef mSetCol

    putCommonToScreen();
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
    pars_.memb = fld->doDraw() ? fld->color(): Color::NoColor()
    mSetCol(leftcolsel_,lowfill_);
    mSetCol(rightcolsel_,highfill_);
    mSetCol(wigcolsel_,wigg_);
    mSetCol(reflcolsel_,refline_);
#undef mSetCol

    return true;
}


uiFVVDPropTab::uiFVVDPropTab( uiParent* p, FlatView::Viewer& vwr )
    : uiFlatViewDataDispPropTab(p,vwr,tr("Variable Density"),
      vwr.appearance().ddpars_.vd_.allowuserchangedata_)
    , pars_(ddpars_.vd_)
{
    colseqsel_ = new uiColSeqSel( this, OD::Horizontal,
				    uiStrings::sColorTable() );
    colseqsel_->attach( alignedBelow, lastcommonfld_ );
    colsequsemodesel_ = new uiColSeqUseModeSel( this, false );
    colsequsemodesel_->attach( alignedBelow, colseqsel_ );

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
    ConstRefMan<FlatDataPack> dp = vwr_.getPack( false );
    return BufferString( dp ? dp->name().str() : "" );
}


void uiFVVDPropTab::handleFieldDisplay( bool dodisp )
{
    colseqsel_->display( dodisp );
    colsequsemodesel_->display( dodisp );
}


void uiFVVDPropTab::dispChgCB( CallBacker* )
{
    putToScreen();
}


void uiFVVDPropTab::putToScreen()
{
    colseqsel_->setSeqName( pars_.colseqname_ );
    colsequsemodesel_->setMode( pars_.mapper_->setup().seqUseMode() );
    putCommonToScreen();
    const FlatView::DataDispPars::Common& pars = commonPars();
    Interval<float> range = pars.mapper_->getRange();
    const bool udfrg = mIsUdf(range.start) || mIsUdf(range.stop);
    rgfld_->setValue( udfrg ? vwr_.getDataRange(false) : range );
}


bool uiFVVDPropTab::acceptOK()
{
    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    pars_.mapper_->setup().setSeqUseMode( colsequsemodesel_->mode() );
    if ( pars_.show_ )
	pars_.colseqname_ = colseqsel_->seqName();

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
    uiString lbltxt = tr("Axis '%1'").arg(ad_.name_);
    uiLabel* lbl;
    const bool haveannotchoices = annotnms && annotnms->size() > 1;
    if ( !haveannotchoices )
	lbl = new uiLabel( this, lbltxt );
    else
    {
	annotselfld_ = new uiGenInput( this, lbltxt,
				       StringListInpSpec(*annotnms) );
	lbl = new uiLabel( this, uiStrings::sShow() );
    }

    showannotfld_ = new uiCheckBox( this, uiStrings::sAnnotation() );
    if ( !haveannotchoices )
	showannotfld_->attach( rightOf, lbl );
    else
    {
	showannotfld_->attach( alignedBelow, annotselfld_ );
	lbl->attach( leftOf, showannotfld_ );
    }

    showgridlinesfld_ = new uiCheckBox( this, tr("Grid lines") );
    showgridlinesfld_->attach( rightOf, showannotfld_ );

    if ( dorevertaxis )
    {
	reversedfld_ = new uiCheckBox( this, uiStrings::sReversed() );
	reversedfld_->attach( rightOf, showgridlinesfld_ );
    }

    auxlblfld_ = new uiLabel( this, ad_.auxlabel_ );
    showauxannotfld_ =
	new uiCheckBox( this, tr("Show Annotation"),
			mCB(this,uiFVAnnotPropTab::AxesGroup,showAuxCheckedCB));
    showauxannotfld_->attach( alignedBelow, showannotfld_ );
    auxlblfld_->attach( leftTo, showauxannotfld_ );
    uiSelLineStyle::Setup su;
    su.width( false );
    auxlinestylefld_ = new uiSelLineStyle( this, ad_.auxlinestyle_, su );
    auxlinestylefld_->attach( alignedBelow, showauxannotfld_ );

    setHAlignObj( showannotfld_ );
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
    auxlblfld_->display( hasauxdata );
    showauxannotfld_->display( hasauxdata );
    auxlinestylefld_->display( hasauxdata, true );
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
    if ( annotselfld_ ) annotselfld_->setValue( selannot );
}


uiFVAnnotPropTab::uiFVAnnotPropTab( uiParent* p, FlatView::Viewer& vwr,
				    const BufferStringSet* annots )
    : uiFlatViewPropTab(p,vwr,uiStrings::sAnnotation())
    , annot_(app_.annot_)
    , auxnamefld_( 0 )
    , currentaux_( 0 )
{
    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_);
    if ( uivwr )
	mAttachCB( uivwr->annotChanged, uiFVAnnotPropTab::annotChgdCB );

    colfld_ = new uiColorInput( this, uiColorInput::Setup(annot_.color_).
				lbltxt(tr("Annotation color")),
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
	    : OD::MarkerStyle2D();
	x1rgs_ += auxdata.x1rg_ ? *auxdata.x1rg_ : Interval<double>( 0, 1 );
	x2rgs_ += auxdata.x2rg_ ? *auxdata.x2rg_ : Interval<double>( 0, 1 );
	auxnames.add( mFromUiStringTodo(auxdata.name_) );
    }

    if ( !auxnames.size() )
	return;

    auxnamefld_ = new uiGenInput( this, tr("Aux data"),
				  StringListInpSpec( auxnames ) );
    auxnamefld_->valuechanged.notify( mCB( this, uiFVAnnotPropTab, auxNmFldCB));
    auxnamefld_->attach( alignedBelow, x2_ );

    linestylefld_ = new uiSelLineStyle( this, linestyles_[0],
					tr("Line style") );
    linestylefld_->attach( alignedBelow, auxnamefld_ );

    uiSelLineStyle::Setup su( tr("Line style") );
			  su.color( false );
    linestylenocolorfld_ = new uiSelLineStyle( this, linestyles_[0], su );
    linestylenocolorfld_->attach( alignedBelow, auxnamefld_ );

    fillcolorfld_ = new uiColorInput( this, uiColorInput::Setup(fillcolors_[0]),
				      "Fill color" );

    fillcolorfld_->attach( alignedBelow, linestylefld_ );

    x1rgfld_ = new uiGenInput( this, tr("X-Range"),
			       FloatInpIntervalSpec() );
    x1rgfld_->attach( alignedBelow, fillcolorfld_ );

    x2rgfld_ = new uiGenInput( this, tr("Y-Range"),
			       FloatInpIntervalSpec() );
    x2rgfld_->attach( alignedBelow, x1rgfld_ );
}


uiFVAnnotPropTab::~uiFVAnnotPropTab()
{
    detachAllNotifiers();
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
	    vwr_.getAuxData(indices_[idx])->markerstyles_[0]
	    : OD::MarkerStyle2D();
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
    : uiTabStackDlg(p,uiDialog::Setup(tr("Specify Display Properties"),
				      mNoDlgTitle,
				      mODHelpKey(mODViewer2DPropDlgHelpID))
				      .modal(false))
    , vwr_(vwr)
    , applycb_(applcb)
    , selannot_(selannot)
    , wvatab_(0)
    , annottab_(0)
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
	annottab_ = new uiFVAnnotPropTab( tabParent(), vwr_, annots );
	addGroup( annottab_ );
    }

    titlefld_ = new uiGenInput( this, uiStrings::sTitle() );
    titlefld_->attach( centeredAbove, tabObject() );

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
			     mCB(this,uiFlatViewPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton();

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

    titlefld_->setText( mFromUiStringTodo(vwr_.appearance().annot_.title_) );
    if ( annottab_ )
	annottab_->setSelAnnot( selannot_ );
}


void uiFlatViewPropDlg::doApply( CallBacker* cb )
{
    acceptOK();
    applycb_.doCall( this );
}


bool uiFlatViewPropDlg::rejectOK()
{
    putAllToScreen();
    return true;
}


bool uiFlatViewPropDlg::acceptOK()
{
    mDynamicCastGet(uiFlatViewer*,uivwr,&vwr_);
    if ( !uivwr ) return false;

    NotifyStopper ns1( uivwr->dispParsChanged );
    NotifyStopper ns2( uivwr->annotChanged );

    if ( (wvatab_ && !wvatab_->doDisp()) && (vdtab_ && !vdtab_->doDisp()) )
    {
	uiMSG().error( tr("No data selected for Wiggle or VD display.") );
	return false;
    }

    if ( !uiTabStackDlg::acceptOK() )
	return false;

    vwr_.appearance().annot_.title_ = toUiString(titlefld_->text());
    if ( annottab_ )
	selannot_ = annottab_->getSelAnnot();

    return true;
}


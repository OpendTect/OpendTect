/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Dec 2006
 RCS:           $Id: uiflatviewpropdlg.cc,v 1.15 2007-08-28 20:39:13 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatviewpropdlg.h"
#include "uiflatviewproptabs.h"

#include "coltabedit.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uisellinest.h"
#include "uiseparator.h"

#include "datapackbase.h"


uiFlatViewPropTab::uiFlatViewPropTab( uiParent* p, FlatView::Viewer& vwr,
				      const char* lbl )
    : uiDlgGroup(p,lbl)
    , vwr_(vwr)
    , app_(vwr.appearance())
{
}


uiFlatViewDataDispPropTab::uiFlatViewDataDispPropTab( uiParent* p,
		FlatView::Viewer& vwr, const char* tablbl )
    : uiFlatViewPropTab(p,vwr,tablbl)
    , ddpars_(app_.ddpars_)
{
    dispfld_ = new uiLabeledComboBox( this, "Display" );
    dispfld_->box()->selectionChanged.notify(
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
    return dispfld_->box()->currentItem() != 0;
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


void uiFlatViewDataDispPropTab::setDataNames( const FlatView::Data& fvd )
{
    dispfld_->box()->empty();
    dispfld_->box()->addItem( "No" );
    if ( fvd.arr(true) )
    {
	const char* wvanm = fvd.name( true );
	if ( !wvanm || !*wvanm ) wvanm = "Original WVA data";
	dispfld_->box()->addItem( wvanm );
    }
    if ( fvd.arr(false) )
    {
	const char* vdnm = fvd.name( false );
	if ( !vdnm || !*vdnm ) vdnm = "Original VD data";
	if ( !dispfld_->box()->isPresent(vdnm) )
	    dispfld_->box()->addItem( vdnm );
    }
}


void uiFlatViewDataDispPropTab::putCommonToScreen()
{
    const FlatView::DataDispPars::Common& pars = commonPars();
    bool havedata = pars.show_;
    if ( havedata )
    {
	const char* nm = dataName();
	if ( dispfld_->box()->isPresent( nm ) )
	    dispfld_->box()->setText( dataName() );
	else
	    havedata = false;
    }
    if ( !havedata )
	dispfld_->box()->setCurrentItem( 0 );

    useclipfld_->setValue( mIsUdf( pars.rg_.start ) );
    clipratiofld_->setValue( pars.clipperc_ );
    rgfld_->setValue( pars.rg_ );
    blockyfld_->setValue( pars.blocky_ );
    dispSel( 0 );
}


void uiFlatViewDataDispPropTab::doSetData( const FlatView::Data& fvd, bool wva )
{
    if ( dispfld_->box()->currentItem() == 0 )
	{ vwr_.data().setEmpty( wva ); return; }

    const BufferString datanm( dispfld_->box()->text() );
    FlatView::PackData& pdta = wva ? vwr_.data().wva_ : vwr_.data().vd_;
    if ( datanm == fvd.name(true) )
	pdta = fvd.wva_;
    else if ( datanm == fvd.name(false) )
	pdta = fvd.vd_;
}


void uiFlatViewDataDispPropTab::getCommonFromScreen()
{
    FlatView::DataDispPars::Common& pars = commonPars();

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


const char* uiFVWVAPropTab::dataName() const
{
    return vwr_.data().name( true );
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
    havecol = pars_.memb.isVisible(); \
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


const char* uiFVVDPropTab::dataName() const
{
    return vwr_.data().name( false );
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
					FlatView::Annotation::AxisData& ad,
       					const BufferStringSet* annotnms )
    : uiGroup(p,"Axis Data")
    , ad_(ad)
    , annotselfld_(0)
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
    reversedfld_ = new uiCheckBox( this, "Reversed" );
    reversedfld_->attach( rightOf, showgridlinesfld_ );

    setHAlignObj( showannotfld_ );
}


void uiFVAnnotPropTab::AxesGroup::putToScreen()
{
    reversedfld_->setChecked( ad_.reversed_ );
    showannotfld_->setChecked( ad_.showannot_ );
    showgridlinesfld_->setChecked( ad_.showgridlines_ );
}


void uiFVAnnotPropTab::AxesGroup::getFromScreen()
{
    ad_.reversed_ = reversedfld_->isChecked();
    ad_.showannot_ = showannotfld_->isChecked();
    ad_.showgridlines_ = showgridlinesfld_->isChecked();
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


uiFVAnnotPropTab::uiFVAnnotPropTab( uiParent* p, FlatView::Viewer& vwr,
       				    const BufferStringSet* annots )
    : uiFlatViewPropTab(p,vwr,"Annotation")
    , annot_(app_.annot_)
    , auxnamefld_( 0 )
    , currentaux_( 0 )
{
    colfld_ = new uiColorInput( this, annot_.color_, "Annotation color" );
    x1_ = new AxesGroup( this, annot_.x1_, annots );
    x1_->attach( alignedBelow, colfld_ );
    x2_ = new AxesGroup( this, annot_.x2_ );
    x2_->attach( alignedBelow, x1_ );

    BufferStringSet auxnames;
    for ( int idx=0; idx<annot_.auxdata_.size(); idx++ )
    {
	const FlatView::Annotation::AuxData& auxdata = *annot_.auxdata_[idx];
	if ( !auxdata.name_.buf()[0] || !auxdata.editpermissions_ )
	    continue;

	if ( !auxdata.enabled_ && !auxdata.editpermissions_->onoff_ )
	    continue;

	permissions_ += auxdata.editpermissions_;
	enabled_ += auxdata.enabled_;
	linestyles_ += auxdata.linestyle_;
	indices_ += idx;
	fillcolors_ += auxdata.fillcolor_;
	markerstyles_ += auxdata.markerstyle_;
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

    linestylenocolorfld_ = new uiSelLineStyle( this, linestyles_[0],
	    				"Line style", true, false, true  );
    linestylenocolorfld_->attach( alignedBelow, auxnamefld_ );

    fillcolorfld_ = new uiColorInput( this, fillcolors_[0] );
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
	*permissions_[idx] = *annot_.auxdata_[indices_[idx]]->editpermissions_;
	enabled_[idx] = annot_.auxdata_[indices_[idx]]->enabled_;
	linestyles_[idx] = annot_.auxdata_[indices_[idx]]->linestyle_;
	fillcolors_[idx] = annot_.auxdata_[indices_[idx]]->fillcolor_;
	markerstyles_[idx] = annot_.auxdata_[indices_[idx]]->markerstyle_;
	x1rgs_[idx] = annot_.auxdata_[indices_[idx]]->x1rg_
	    ? *annot_.auxdata_[indices_[idx]]->x1rg_
	    : Interval<double>( 0, 1 );
	x2rgs_[idx] = annot_.auxdata_[indices_[idx]]->x2rg_
	    ? *annot_.auxdata_[indices_[idx]]->x2rg_
	    : Interval<double>( 0, 1 );
    }

    if ( auxnamefld_ )
	updateAuxFlds( currentaux_ );
}


void uiFVAnnotPropTab::getFromScreen()
{
    annot_.color_ = colfld_->color();
    x1_->getFromScreen();
    x2_->getFromScreen();

    if ( !auxnamefld_ )
	return;

    getFromAuxFld( currentaux_ );

    for ( int idx=0; idx<indices_.size(); idx++ )
    {
	annot_.auxdata_[indices_[idx]]->linestyle_ = linestyles_[idx];
	annot_.auxdata_[indices_[idx]]->fillcolor_ = fillcolors_[idx];
	annot_.auxdata_[indices_[idx]]->markerstyle_ = markerstyles_[idx];
	annot_.auxdata_[indices_[idx]]->enabled_ = enabled_[idx];
	if ( annot_.auxdata_[indices_[idx]]->x1rg_ )
	    *annot_.auxdata_[indices_[idx]]->x1rg_ = x1rgs_[idx];
	if ( annot_.auxdata_[indices_[idx]]->x2rg_ )
	    *annot_.auxdata_[indices_[idx]]->x2rg_ = x2rgs_[idx];
    }
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
	 annot_.auxdata_[indices_[idx]]->close_)
    {
	fillcolorfld_->setColor( fillcolors_[idx] );
	fillcolorfld_->display( true );
    }
    else
	fillcolorfld_->display( false );

    if ( permissions_[idx]->x1rg_ && annot_.auxdata_[indices_[idx]]->x1rg_ )
    {
	x1rgfld_->setValue( x1rgs_[idx] );
	x1rgfld_->display( true );
    }
    else
	x1rgfld_->display( false );

    if ( permissions_[idx]->x2rg_ && annot_.auxdata_[indices_[idx]]->x2rg_ )
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
				      "51.0.0"))
    , vwr_(vwr)
    , applycb_(applcb)
    , initialdata_(vwr.data())
    , selannot_(selannot)
{
    wvatab_ = new uiFVWVAPropTab( tabParent(), vwr_ );
    addGroup( wvatab_ );
    vdtab_ = new uiFVVDPropTab( tabParent(), vwr_ );
    addGroup( vdtab_ );
    annottab_ = new uiFVAnnotPropTab( tabParent(), vwr_, annots );
    addGroup( annottab_ );

    titlefld_ = new uiGenInput( this, "Title" );
    titlefld_->attach( centeredAbove, tabObject() );

    uiPushButton* applybut = new uiPushButton( this, "&Apply",
			     mCB(this,uiFlatViewPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
    enableSaveButton( "Save as Default" );

    putAllToScreen();
}


void uiFlatViewPropDlg::getAllFromScreen()
{
    wvatab_->setData( initialdata_ ); vdtab_->setData( initialdata_ );
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab&,ptab,getGroup(idx))
	ptab.getFromScreen();
    }
    vwr_.appearance().annot_.title_ = titlefld_->text();
    selannot_ = annottab_->getSelAnnot();
}


void uiFlatViewPropDlg::putAllToScreen()
{
    wvatab_->setDataNames( initialdata_ ); vdtab_->setDataNames( initialdata_ );
    for ( int idx=0; idx<nrGroups(); idx++ )
    {
	mDynamicCastGet(uiFlatViewPropTab&,ptab,getGroup(idx))
	ptab.putToScreen();
    }
    titlefld_->setText( vwr_.appearance().annot_.title_ );
    annottab_->setSelAnnot( selannot_ );
}


void uiFlatViewPropDlg::doApply( CallBacker* )
{
    getAllFromScreen();
    applycb_.doCall( this );
}


bool uiFlatViewPropDlg::rejectOK( CallBacker* cb )
{
    vwr_.data() = initialdata_;
    vwr_.syncDataPacks(); // Just to be sure
    return true;
}


bool uiFlatViewPropDlg::acceptOK( CallBacker* cb )
{
    if ( !uiTabStackDlg::acceptOK(cb) )
	return false;

    getAllFromScreen();
    vwr_.syncDataPacks();
    return true;
}

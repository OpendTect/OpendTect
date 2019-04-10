/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uiwelldispprop.h"

#include "uichecklist.h"
#include "uicolor.h"
#include "uicolseqsel.h"
#include "uicolsequsemodesel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uicombobox.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uistrings.h"
#include "welllog.h"
#include "welllogset.h"

static int deflogwidth = 250;

uiWellDispProperties::uiWellDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::BasicDispProps& pr )
    : uiGroup(p,"Well display properties group")
    , props_(&pr)
    , propChanged(this)
    , setup_(su)
    , curwelllogproperty_(0)
{
    szfld_ = new uiLabeledSpinBox( this, su.mysztxt_ );
    szfld_->box()->setInterval( StepInterval<int>(0,100,1) );
    szfld_->box()->setValue( props().size() );
    szfld_->box()->valueChanging.notify(mCB(this,uiWellDispProperties,propChg));
    uiColorInput::Setup csu( props().color() );
    csu.lbltxt( su.mycoltxt_ );
    colfld_ = new uiColorInput( this, csu, toString(su.mycoltxt_) );
    colfld_->attach( alignedBelow, szfld_ );
    colfld_->colorChanged.notify( mCB(this,uiWellDispProperties,propChg) );

    setHAlignObj( colfld_ );
}


void uiWellDispProperties::getFontStyles( uiStringSet& stls )
{
    stls.add( uiStrings::sNormal() )
	.add( tr("Bold") )
	.add( tr("Italic") )
	.add( tr("Bold Italic") );
}

void uiWellDispProperties::getFontData( FontData& fnt, uiSpinBox* ptszfld,
					uiComboBox* fsfld )
{
    fnt.setPointSize( ptszfld->getIntValue() );
    const int fontstyle = fsfld->getIntValue();
    const bool bold = fontstyle==1 || fontstyle==3;
    fnt.setWeight( bold ? FontData::Bold : FontData::Normal );
    fnt.setItalic( fontstyle==2 || fontstyle==3 );
}


void uiWellDispProperties::propChg( CallBacker* )
{
    getFromScreen();
    propChanged.trigger();
}


void uiWellDispProperties::putToScreen()
{
    NotifyStopper ns1( szfld_->box()->valueChanging );
    NotifyStopper ns2( colfld_->colorChanged );
    colfld_->setColor( props().color() );
    doPutToScreen();
    szfld_->box()->setValue( props().size() );
}


void uiWellDispProperties::getFromScreen()
{
    props().setSize( szfld_->box()->getIntValue() );
    props().setColor( colfld_->color() );
    doGetFromScreen();
}


uiWellTrackDispProperties::uiWellTrackDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::TrackDispProps& tp )
    : uiWellDispProperties(p,su,tp)
{
    dispabovefld_ = new uiCheckBox( this, uiStrings::sAbove() );
    dispabovefld_->attach( alignedBelow, colfld_ );
    dispbelowfld_ = new uiCheckBox( this, uiStrings::sBelow() );
    dispbelowfld_->attach( rightOf, dispabovefld_ );
    uiLabel* lbl = new uiLabel( this, tr("Display well name") , dispabovefld_ );
    lbl = new uiLabel( this, uiStrings::sTrack() );
    lbl->attach( rightOf, dispbelowfld_ );

    nmsizefld_ = new uiLabeledSpinBox( this, tr("Name size") );
    nmsizefld_->box()->setInterval( 0, 500, 2 );
    nmsizefld_->attach( alignedBelow, dispabovefld_  );

    uiStringSet fontstyles; getFontStyles( fontstyles );
    nmstylefld_ = new uiComboBox( this, fontstyles, "Fontstyle" );
    nmstylefld_->attach( rightOf, nmsizefld_ );

    nmsizedynamicfld_ = new uiCheckBox( this, uiStrings::sDynamic() );
    nmsizedynamicfld_->attach( rightOf,nmstylefld_ );

    doPutToScreen();

    dispabovefld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmstylefld_->selectionChanged.notify(
	    mCB(this,uiWellTrackDispProperties,propChg) );
    nmsizedynamicfld_->activated.notify(
	mCB(this,uiWellTrackDispProperties,propChg) );

}


void uiWellTrackDispProperties::resetProps( Well::TrackDispProps& pp )
{
    props_ = &pp;
}


void uiWellTrackDispProperties::doPutToScreen()
{
    dispbelowfld_->setChecked( trackprops().dispBelow() );
    dispabovefld_->setChecked( trackprops().dispAbove() );
    nmsizefld_->box()->setValue( trackprops().font().pointSize());
    nmsizedynamicfld_->setChecked( trackprops().nameSizeDynamic() );

    int style = trackprops().font().weight()>FontData::Normal ? 1 : 0;
    if ( trackprops().font().isItalic() )
	style += 2;

    nmstylefld_->setValue( style );
}


void uiWellTrackDispProperties::doGetFromScreen()
{
    trackprops().setDispBelow( dispbelowfld_->isChecked() );
    trackprops().setDispAbove( dispabovefld_->isChecked() );
    trackprops().setNameSizeDynamic( nmsizedynamicfld_->isChecked() );
    FontData fnt( trackprops().font() );
    getFontData( fnt, nmsizefld_->box(), nmstylefld_ );
    trackprops().setFont( fnt );
}


uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::MarkerDispProps& mp,
				const BufferStringSet& allmarkernms )
    : uiWellDispProperties(p,su,mp)
{
    uiStringSet shapes3d;
    uiStringSet shapes2d;
    shapes3d.add(tr("Cylinder"))
	    .add(uiStrings::sSquare())
	    .add(uiStrings::sSphere());
    shapes2d.add(tr("Dot"))
	    .add(tr("Solid"))
	    .add(tr("Dash"));

    shapefld_ = new uiLabeledComboBox( this, uiStrings::sShape() );
    shapefld_->attach( alignedBelow, colfld_ );
    for ( int idx=0; idx<shapes3d.size(); idx++)
	shapefld_->box()->addItem(
		setup_.onlyfor2ddisplay_ ? shapes2d[idx] : shapes3d[idx]);

    cylinderheightfld_ = new uiLabeledSpinBox( this, uiStrings::sHeight() );
    cylinderheightfld_->box()->setInterval( 0, 10, 1 );
    cylinderheightfld_->attach( rightOf, shapefld_ );
    cylinderheightfld_->display( !setup_.onlyfor2ddisplay_ );

    singlecolfld_ = new uiCheckBox( this, tr("use single color") );
    singlecolfld_->attach( rightOf, colfld_);
    colfld_->setSensitive( singlecolfld_->isChecked() );

    nmsizefld_ = new uiLabeledSpinBox( this, tr("Names size") );
    nmsizefld_->box()->setInterval( 0, 500, 1 );
    nmsizefld_->box()->setValue( 2 * mp.size() );
    nmsizefld_->attach( alignedBelow, shapefld_ );
    nmsizefld_->display( !setup_.onlyfor2ddisplay_ );

    uiStringSet styles; getFontStyles( styles );
    nmstylefld_ = new uiComboBox( this, styles, "Fontstyle" );
    nmstylefld_->attach( rightOf, nmsizefld_ );
    nmstylefld_->display( !setup_.onlyfor2ddisplay_ );

    nmsizedynamicfld_ = new uiCheckBox( this,  uiStrings::sDynamic() );
    nmsizedynamicfld_->attach( rightOf, nmstylefld_ );

    uiString dlgtxt = tr("Color for Names");
    uiColorInput::Setup csu( mrkprops().color() ); csu.lbltxt( dlgtxt );
    nmcolfld_ = new uiColorInput( this, csu, toString(dlgtxt) );
    nmcolfld_->attach( alignedBelow, nmsizefld_ );
    nmcolfld_->display( !setup_.onlyfor2ddisplay_ );

    samecolasmarkerfld_ = new uiCheckBox( this, tr("same as markers") );
    samecolasmarkerfld_->attach( rightOf, nmcolfld_);
    samecolasmarkerfld_->display( !setup_.onlyfor2ddisplay_ );

    uiListBox::Setup msu( OD::ChooseZeroOrMore, tr("Display markers") );
    displaymarkersfld_ = new uiListBox( this, msu );
    displaymarkersfld_->addItems( allmarkernms );
    if ( !setup_.onlyfor2ddisplay_ )
	displaymarkersfld_->attach( alignedBelow, nmcolfld_ );
    else
	displaymarkersfld_->attach( alignedBelow, shapefld_ );

    doPutToScreen();
    markerFldsChgd(0);

    CallBack propcb = mCB(this,uiWellMarkersDispProperties,propChg);
    CallBack mrkrcb = mCB(this,uiWellMarkersDispProperties,markerFldsChgd);

    cylinderheightfld_->box()->valueChanging.notify( propcb );
    nmcolfld_->colorChanged.notify( propcb );
    nmsizefld_->box()->valueChanging.notify( propcb );
    nmstylefld_->selectionChanged.notify( propcb );
    samecolasmarkerfld_->activated.notify( propcb );
    samecolasmarkerfld_->activated.notify( mrkrcb );
    singlecolfld_->activated.notify( propcb );
    nmsizedynamicfld_->activated.notify( propcb );
    shapefld_->box()->selectionChanged.notify( propcb );
    shapefld_->box()->selectionChanged.notify( mrkrcb );
    displaymarkersfld_->itemChosen.notify( propcb );
    displaymarkersfld_->itemChosen.notify( mrkrcb );
}


void uiWellMarkersDispProperties::getSelNames()
{
    BufferStringSet selnms;
    for ( int idx=0; idx<displaymarkersfld_->size(); idx++ )
    {
	if ( displaymarkersfld_->isChosen( idx ) )
	    selnms.add( displaymarkersfld_->itemText(idx) );
    }
    mrkprops().setSelMarkerNames( selnms );
}


void uiWellMarkersDispProperties::setSelNames()
{
    NotifyStopper ns( displaymarkersfld_->itemChosen );
    displaymarkersfld_->setChosen( mrkprops().selMarkerNames() );
}


void uiWellMarkersDispProperties::setAllMarkerNames(
				const BufferStringSet& markernms,
				const TypeSet<Color>& markercols )
{
    displaymarkersfld_->setEmpty();
    displaymarkersfld_->addItems( markernms );
    for ( int idx=0; idx<markercols.size(); idx++ )
	displaymarkersfld_->setPixmap( idx, markercols[idx] );
    setSelNames();
}


void uiWellMarkersDispProperties::resetProps( Well::MarkerDispProps& mdp )
{
    props_ = &mdp;
    setSelNames();
}


void uiWellMarkersDispProperties::markerFldsChgd( CallBacker* cb )
{
    colfld_->setSensitive( singlecolfld_->isChecked() );
    nmcolfld_->setSensitive( !samecolasmarkerfld_->isChecked() );
    cylinderheightfld_->display( !shapefld_->box()->currentItem() &&
				 !setup_.onlyfor2ddisplay_ );
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    NotifyStopper ns1( cylinderheightfld_->box()->valueChanging );

    shapefld_->box()->setCurrentItem( mrkprops().shapeType() );
    cylinderheightfld_->box()->setValue( mrkprops().cylinderHeight() );
    singlecolfld_->setChecked( mrkprops().singleColor() );
    nmsizedynamicfld_->setChecked( mrkprops().nameSizeDynamic() );
    const int sz = mrkprops().font().pointSize();
    if ( sz > 0 )
	nmsizefld_->box()->setValue( sz );

    int style = mrkprops().font().weight()>FontData::Normal ? 1 : 0;
    if ( mrkprops().font().isItalic() )
	style += 2;

    nmstylefld_->setValue( style );

    samecolasmarkerfld_->setChecked( mrkprops().sameNameCol() );
    nmcolfld_->setColor( mrkprops().nameColor() );
    setSelNames();
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().setShapeType( shapefld_->box()->currentItem() );
    mrkprops().setCylinderHeight( cylinderheightfld_->box()->getIntValue() );
    mrkprops().setSingleColor( singlecolfld_->isChecked() );
    mrkprops().setNameSizeDynamic( nmsizedynamicfld_->isChecked() );
    FontData fnt( mrkprops().font() );
    getFontData( fnt, nmsizefld_->box(), nmstylefld_ );
    mrkprops().setFont( fnt );
    mrkprops().setSameNameCol( samecolasmarkerfld_->isChecked() );
    mrkprops().setNameColor( nmcolfld_->color() );
    getSelNames();
}


uiWellLogDispProperties::uiWellLogDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::LogDispProps& lp, const Well::LogSet* wl)
    : uiWellDispProperties(p,su,lp)
{

    stylefld_ = new uiCheckList( this, uiCheckList::OneOnly, OD::Horizontal );
    stylefld_->addItem( uiStrings::sWellLog() )
	      .addItem( uiStrings::sSeismic() );
    if ( !setup_.onlyfor2ddisplay_ )
	stylefld_->addItem( tr("Log tube") );
    stylefld_->setLabel( uiStrings::sStyle() );
    stylefld_->attach( alignedAbove, szfld_ );

    uiSeparator* sep1 = new uiSeparator( this, "Sep" );
    sep1->attach( stretchedAbove, stylefld_ );

    rangefld_ = new uiGenInput( this, tr("Log range (min/max)"),
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, stylefld_ );
    sep1->attach( stretchedBelow, rangefld_ );

    uiStringSet choices;
    choices.add( tr("Clip Rate") ).add( uiStrings::sDataRange() );

    cliprangefld_ = new uiGenInput( this, uiStrings::sSpecify(),
		StringListInpSpec(choices));
    cliprangefld_->attach( alignedAbove, rangefld_ );

    clipratefld_ = new uiGenInput( this, tr("Clip rate"), StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedBelow, cliprangefld_ );

    logarithmfld_ = new uiCheckBox( this, uiStrings::sLogarithmic() );
    logarithmfld_->attach( rightOf, rangefld_ );

    revertlogfld_ = new uiCheckBox( this, uiStrings::sFlip() );
    revertlogfld_->attach( rightOf, cliprangefld_ );

    lblr_ = new uiLabeledSpinBox( this, tr("Repeat") );
    repeatfld_ = lblr_ ->box();
    repeatfld_->setInterval( 1, 20, 1 );
    lblr_->attach( alignedBelow, colfld_ );

    logsfld_ = new uiLabeledComboBox( this, uiStrings::sLog() );
    logsfld_->box()->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedAbove, cliprangefld_ );

    logfilltypefld_ = new uiLabeledComboBox( this, tr("Fill"));
    logfilltypefld_->box()->addItem( uiStrings::sNone() );
    logfilltypefld_->box()->addItem( tr("Left of log") );
    logfilltypefld_->box()->addItem( tr("Right of log") );
    logfilltypefld_->box()->addItem( tr("Full panel") );
    logfilltypefld_->attach( alignedBelow, colfld_ );

    filllogsfld_ = new uiLabeledComboBox( this, tr("Fill with") );
    filllogsfld_->attach( alignedBelow, logfilltypefld_ );

    singlfillcolfld_ = new uiCheckBox( this, tr("single color") );
    singlfillcolfld_->attach( rightOf, logfilltypefld_ );


    colseqselfld_ = new uiColSeqSel( this, OD::Horizontal,
					uiStrings::sColorTable() );
    colseqselfld_->attach( alignedBelow, filllogsfld_ );

    colorrangefld_ = new uiGenInput( this, uiString::empty(),
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    colorrangefld_->attach( rightOf, colseqselfld_ );

    sequsefld_ = new uiColSeqUseModeSel( this, false, uiString::empty() );
    sequsefld_->attach( rightOf, colorrangefld_ );

    uiSeparator* sep2 = new uiSeparator( this, "Sep" );
    sep2->attach( stretchedBelow, colseqselfld_ );

    const uiString lbl = tr("Log display width").withSurvXYUnit();
    logwidthslider_ = new uiSlider( this, uiSlider::Setup(lbl).withedit(true) );
    logwidthslider_->attach( alignedBelow, colseqselfld_ );
    logwidthslider_->attach( ensureBelow, sep2 );

    logwidthslider_->setMinValue( 0.f );
    logwidthslider_->setMaxValue( 10000.0f );
    logwidthslider_->setStep( 250.0f );

    seiscolorfld_ = new uiColorInput( this,
				 uiColorInput::Setup(logprops().seisColor())
				.lbltxt(tr("Filling color")) );
    seiscolorfld_->attach( alignedBelow, lblr_ );
    seiscolorfld_->display(false);

    lblo_ = new uiLabeledSpinBox( this, tr("Overlap") );
    ovlapfld_ = lblo_->box();
    ovlapfld_->setInterval( 0, 100, 20 );
    lblo_->attach( rightOf, lblr_ );

    fillcolorfld_ = new uiColorInput( this,
				 uiColorInput::Setup(logprops().seisColor())
				.lbltxt(tr("Filling color")) );
    fillcolorfld_->attach( alignedBelow, logfilltypefld_ );
    fillcolorfld_->display(false);

    setLogSet( wl );
    putToScreen();

    CallBack propchgcb = mCB(this,uiWellLogDispProperties,propChg);
    CallBack choiceselcb = mCB(this,uiWellLogDispProperties,choiceSel);
    cliprangefld_->valuechanged.notify( choiceselcb );
    clipratefld_->valuechanged.notify( choiceselcb );
    colorrangefld_->valuechanged.notify( choiceselcb );
    rangefld_->valuechanged.notify( choiceselcb );
    clipratefld_->valuechanged.notify( propchgcb );
    colseqselfld_->seqChanged.notify( propchgcb );
    colorrangefld_->valuechanged.notify( propchgcb );
    fillcolorfld_->colorChanged.notify( propchgcb );
    logwidthslider_->valueChanged.notify(propchgcb);

    logarithmfld_->activated.notify( propchgcb );
    ovlapfld_->valueChanging.notify( propchgcb );
    rangefld_->valuechanged.notify( propchgcb );
    repeatfld_->valueChanging.notify( propchgcb );
    revertlogfld_->activated.notify( propchgcb );
    seiscolorfld_->colorChanged.notify( propchgcb );
    singlfillcolfld_->activated.notify( propchgcb );
    stylefld_->changed.notify( propchgcb );
    logfilltypefld_->box()->selectionChanged.notify( propchgcb );
    sequsefld_->modeChange.notify( propchgcb );
    filllogsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,logSel) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    logfilltypefld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel));
    singlfillcolfld_->activated.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel) );

    stylefld_->changed.notify(
	mCB(this,uiWellLogDispProperties,isStyleChanged) );
}


void uiWellLogDispProperties::resetProps( Well::LogDispProps& ldp )
{
    props_ = &ldp;
}


#define mSetSwapFillIdx( fidx )\
	if ( logprops().revertLog() )\
	{ if ( fidx == 2 ) fidx = 1; else if ( fidx == 1 ) fidx =2; }

void uiWellLogDispProperties::doPutToScreen()
{
    NotifyStopper nssfc( singlfillcolfld_->activated );
    NotifyStopper nso( ovlapfld_->valueChanging );
    NotifyStopper nsr( repeatfld_->valueChanging );
    NotifyStopper nsl( logarithmfld_->activated );
    NotifyStopper nsrev( revertlogfld_->activated );
    NotifyStopper nsslf( logfilltypefld_->box()->selectionChanged );
    NotifyStopper nsstylefld( stylefld_->changed );

    revertlogfld_->setChecked( logprops().revertLog() );
    logsfld_->box()->setText( logprops().logName() );
    rangefld_->setValue( logprops().range() );
    colorrangefld_->setValue( logprops().fillRange() );
    filllogsfld_->box()-> setText( logprops().fillName() );

    stylefld_->setChecked( logprops().style(), true );
    logarithmfld_->setChecked( logprops().isLogarithmic() );
    colseqselfld_->setSeqName( logprops().seqName() );
    sequsefld_->setMode( logprops().seqUseMode() );
    ovlapfld_->setValue( logprops().repeatOverlap() );
    repeatfld_->setValue( logprops().repeat() );
    int fidx = logprops().fillLeft() ? logprops().fillRight() ? 3 : 1
				     : logprops().fillRight() ? 2 : 0;
    mSetSwapFillIdx( fidx )

    logfilltypefld_->box()->setCurrentItem( fidx );
    singlfillcolfld_->setChecked( logprops().singleColor() );
    clipratefld_->setValue( logprops().clipRate() );
    cliprangefld_->setValue( logprops().isDataRange() );
    const float cliprate = logprops().clipRate();
    if ( mIsUdf(cliprate) || cliprate > 100  )
    {
	cliprangefld_->setValue( true );
	clipratefld_->setValue( 0.0 );
    }

    logwidthslider_->setValue( logprops().logWidth() );

    if ( logprops().style() != 1 )
	fillcolorfld_->setColor( logprops().seisColor() );
    else
	seiscolorfld_->setColor( logprops().seisColor() );

    logSel( 0 );
    isStyleChanged( 0 );
    choiceSel( 0 );
}


void uiWellLogDispProperties::doGetFromScreen()
{
    logprops().setStyle( stylefld_->firstChecked() );

    bool isdatarange = cliprangefld_->getBoolValue();
    float cliprate = clipratefld_->getFValue();
    if ( mIsUdf(cliprate) || cliprate > 100 )
	{ cliprate = 0.0f; isdatarange = true; }
    logprops().setClipRate( cliprate );
    logprops().setIsDataRange( isdatarange );

    Interval<float> rg = rangefld_->getFInterval();
    bool isreverted = revertlogfld_->isChecked();
    if ( !rg.isRev() && isreverted )
	rg.sort( false );
    if ( rg.isRev() && !isreverted )
	rg.sort( true );
    logprops().setRange( rg );
    logprops().setRevertLog( isreverted );

    logprops().setFillRange( colorrangefld_->getFInterval() );
    logprops().setIsLogarithmic( logarithmfld_->isChecked() );
    logprops().setSingleColor( singlfillcolfld_->isChecked() );
    int fillidx = logfilltypefld_->box()->currentItem();
    mSetSwapFillIdx( fillidx )
    logprops().setFillLeft( fillidx == 1 || fillidx == 3 );
    logprops().setFillRight( fillidx == 2 || fillidx == 3 );
    logprops().setSeqName( colseqselfld_->seqName() );
    logprops().setSeqUseMode( sequsefld_->mode() );
    logprops().setRepeat( stylefld_->isChecked(1) ? repeatfld_->getIntValue()
						  : 1 );
    logprops().setRepeatOverlap( mCast( float, ovlapfld_->getIntValue() ) );
    logprops().setSeisColor( logprops().style() == 1 ? seiscolorfld_->color()
						     : fillcolorfld_->color() );
    logprops().setLogName( logsfld_->box()->text() );
    logprops().setFillName( filllogsfld_->box()->text() );

    deflogwidth = (int)logwidthslider_->getValue();
    logprops().setLogWidth( deflogwidth );

    if ( !setup_.onlyfor2ddisplay_ && curwelllogproperty_ &&
	    curwelllogproperty_!=this )
    {
	if ( curwelllogproperty_->logprops().style() == 2 ||
	     ( curwelllogproperty_->logprops().style() != 2 &&
	       logprops().style() == 2 ) )
	{
	    logprops().setLogName( "None" );
	}
    }
}


void uiWellLogDispProperties::isFilledSel( CallBacker* )
{
    const bool iswelllogortube = !stylefld_->isChecked( 1 );
    const bool issinglecol = singlfillcolfld_->isChecked();
    const int fillidx = logfilltypefld_->box()->currentItem();
    const bool isleftfilled_ = fillidx == 1 || fillidx == 3;
    const bool isrightfilled_ = fillidx == 2 || fillidx == 3;
    const bool isfilled = isrightfilled_ || isleftfilled_;
    singlfillcolfld_->display( isfilled && iswelllogortube );
    colseqselfld_->display( iswelllogortube &&  isfilled && !issinglecol );
    seiscolorfld_->display( !iswelllogortube );
    fillcolorfld_->display( iswelllogortube && issinglecol && isfilled );
    filllogsfld_->display( iswelllogortube &&  isfilled && !issinglecol );
    colorrangefld_->display( iswelllogortube &&  isfilled && !issinglecol );
    sequsefld_->display( isfilled && iswelllogortube && !issinglecol );
}


void uiWellLogDispProperties::disableLogDisplays()
{
    singlfillcolfld_->display( false );
    colseqselfld_->display( false );
    seiscolorfld_->display( false );
    fillcolorfld_->display( false );
    filllogsfld_->display( false );
    colorrangefld_->display( false );
    sequsefld_->display( false );
    lblr_->display( false );
    lblo_->display( false );
    singlfillcolfld_->display( false );
    logwidthslider_->display( false );
    szfld_->display( false );
    colfld_->display( false );
    sequsefld_->display( false );
    revertlogfld_->display( false );
    logfilltypefld_->display( false );
}


void uiWellLogDispProperties::setSeismicSel()
{
    setStyleSensitive( true );
    lblr_->display( !setup_.onlyfor2ddisplay_ );
    lblo_->display( !setup_.onlyfor2ddisplay_ );
    colfld_->display( true );
    szfld_->display( true );
    logwidthslider_->display( !setup_.onlyfor2ddisplay_ );
    isFilledSel(0);
}


void uiWellLogDispProperties::setWellLogSel()
{
    BufferString sel = logsfld_->box()->text();
    if ( sel == "None" || sel == "none" )
	setStyleSensitive( false );
    else
	setStyleSensitive( true );

    singlfillcolfld_->display( true );
    colseqselfld_->display( true );
    logfilltypefld_->display( true );
    filllogsfld_->display( true );
    colorrangefld_->display( true );
    sequsefld_->display( true );
    revertlogfld_->display( true );
    logwidthslider_->display( !setup_.onlyfor2ddisplay_ );
    colfld_->display( true );
    szfld_->display( true );
    isFilledSel(0);
}

const char* uiWellLogDispProperties::logName() const
{
    return logsfld_->box()->text();
}

void uiWellLogDispProperties::setTubeSel()
{
    setWellLogSel();
}


void uiWellLogDispProperties::setStyleSensitive( bool yn )
{
    colfld_->setSensitive( yn );
    szfld_->setSensitive( yn );
    singlfillcolfld_->setSensitive( yn );
    logfilltypefld_->setSensitive( yn );
}


void uiWellLogDispProperties::isStyleChanged( CallBacker* )
{
    disableLogDisplays();

    const int style = logprops().style();

    if ( style == 0 )
	setWellLogSel();
    else if ( style == 1 )
	setSeismicSel();
    else
	setTubeSel();
}


void uiWellLogDispProperties::setLogSet( const Well::LogSet* wls )
{
    wls_ = wls;
    const BufferString curlognm = logsfld_->box()->text();
    BufferStringSet lognames;
    wls_->getNames( lognames );
    lognames.sort();
    logsfld_->box()->setEmpty();
    logsfld_->box()->addItem(uiStrings::sNone());
    logsfld_->box()->addItems( lognames );
    filllogsfld_->box()->setEmpty();
    filllogsfld_->box()->addItems( lognames );
    if ( lognames.isPresent(curlognm) )
    {
	logsfld_->box()->setText( curlognm );
	filllogsfld_->box()->setText( curlognm );
    }
    logSel( 0 );
}


void uiWellLogDispProperties::logSel( CallBacker* cb )
{
    setFieldVals();
    const BufferString logname = logsfld_->box()->text();
    logprops().setLogName( logname );

}


void uiWellLogDispProperties::selNone()
{
    rangefld_->setValue( Interval<float>(0,0) );
    colorrangefld_->setValue( Interval<float>(0,0) );
    colfld_->setColor( logprops().color() );
    seiscolorfld_->setColor( logprops().seisColor() );
    fillcolorfld_->setColor( logprops().seisColor() );
    stylefld_->setChecked( 0, true );
    setFldSensitive( false );
    cliprangefld_->setValue( true );
    clipratefld_->setValue( 0.0 );
    repeatfld_->setValue( 0 );
    ovlapfld_->setValue( 0 );
    singlfillcolfld_->setChecked( false );
    colseqselfld_->setSeqName( logprops().seqName() );
    logwidthslider_->setValue( deflogwidth );
}



void uiWellLogDispProperties::setFldSensitive( bool yn )
{
    rangefld_->setSensitive( yn );
    colorrangefld_->setSensitive( yn );
    cliprangefld_->setSensitive( yn );
    revertlogfld_->setSensitive( yn );
    colfld_->setSensitive( yn );
    seiscolorfld_->setSensitive( yn );
    fillcolorfld_->setSensitive( yn );
    stylefld_->setSensitive( yn );
    clipratefld_->setSensitive( yn );
    lblr_->setSensitive( yn );
    szfld_->setSensitive( yn );
    singlfillcolfld_->setSensitive( yn );
    colseqselfld_->setSensitive( yn );
    filllogsfld_->setSensitive(yn);
    sequsefld_->setSensitive( yn );
    logarithmfld_->setSensitive(yn);
    logwidthslider_->setSensitive(yn);
    logfilltypefld_->setSensitive(yn);
}


void uiWellLogDispProperties::choiceSel( CallBacker* )
{
    const int isdatarange = cliprangefld_->getBoolValue();
    rangefld_->display( isdatarange );
    clipratefld_->display( !isdatarange );
}


void uiWellLogDispProperties::setFieldVals()
{
    BufferString sel = logsfld_->box()->text();
    if ( sel == "None" || sel == "none" )
    {
	selNone();
	return;
    }
    setFldSensitive( true );
}


void uiWellLogDispProperties::updateRange( CallBacker* )
{
    const char* lognm = logsfld_->box()->itemText(
			logsfld_->box()->currentItem() );
    const Well::Log* wl = wls_->getLogByName( lognm );
    if ( wl )
    {
	rangefld_->setValue( wl->valueRange() );
	propChanged.trigger();
    }
}


void uiWellLogDispProperties::updateFillRange( CallBacker* )
{
    const char* lognm = filllogsfld_->box()->itemText(
			filllogsfld_->box()->currentItem() );
    const Well::Log* wl = wls_->getLogByName( lognm );
    if ( wl )
    {
	colorrangefld_->setValue( wl->valueRange() );
	propChanged.trigger();
    }
}



void uiWellLogDispProperties::calcRange( const char* lognm,
					 Interval<float>& valr )
{
    valr.set( mUdf(float), -mUdf(float) );
    const Well::Log* wl = wls_->getLogByName( lognm );
    if ( wl )
	valr = wl->valueRange();
    else
	valr.start = valr.stop = mUdf(float);
}

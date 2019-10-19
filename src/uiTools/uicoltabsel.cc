/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicoltabsel.h"
#include "uicolseqdisp.h"
#include "uicolsequsemodesel.h"
#include "uicolseqman.h"
#include "coltabmapper.h"
#include "datadistributiontools.h"
#include "settings.h"
#include "uisettings.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uifunctiondisplay.h"

const char* uiColTabSelTool::sKeyEnableAsymmetricClipping()
	{ return "dTect.Disp.ColTab.Enable Asymmetric Clipping"; }
const char* uiColTabSelTool::sKeyShowTextManipulators()
	{ return "dTect.Disp.ColTab.Show Text Manipulators"; }
const char* uiColTabSelTool::sKeyShowUseModeSel()
	{ return "dTect.Disp.ColTab.Show Flip/Cyclic Tool"; }
const char* uiColTabSelTool::sKeyShowHistEqBut()
	{ return "dTect.Disp.ColTab.Show Histogram Equalisation Switch"; }

static const char* sColTabSettFactKy = "ColTab";

static uiColTabSelTool* globseltool_ = 0;
mExtern(uiTools) void SetuiCOLTAB( uiColTabSelTool* st )
{
    globseltool_ = st;
}
uiColTabSelTool& uiCOLTAB()
{
    return *globseltool_;
}


class uiEdMapperSetupDlg : public uiDialog
{ mODTextTranslationClass(uiEdMapperSetupDlg);
public:

uiEdMapperSetupDlg( uiColTabSelTool& st )
    : uiDialog( st.asParent(),
	    uiDialog::Setup(tr("Ranges/Clipping"),mNoDlgTitle,
			 mODHelpKey(mAutoRangeClipDlgHelpID))
	    .modal(false).savebutton(true) )
    , seltool_(st)
{
    dispbothclips_ = Settings::common().isTrue(
		     uiColTabSelTool::sKeyEnableAsymmetricClipping() );
    showAlwaysOnTop();
    setDeleteOnClose( true );
    setOkCancelText( uiStrings::sApply(), uiStrings::sClose() );

    rangefld_ = new uiGenInput( this, tr("Fixed scale range"),
				      FloatInpIntervalSpec() );
    rangefld_->setWithCheck( true );

    if ( !dispbothclips_ )
	clipfld_ = new uiGenInput( this, tr("Percentage clipped"),
				  FloatInpSpec(1.f,0.f,100.f,0.1f) );
    else
    {
	clipfld_ = new uiGenInput( this, tr("Percentages clipped (low/high)"),
				  FloatInpIntervalSpec() );
	clipfld_->setElemSzPol( uiObject::Small );
    }
    clipfld_->attach( alignedBelow, rangefld_ );

    usemodefld_ = new uiColSeqUseModeSel( this, false );
    usemodefld_->attach( alignedBelow, clipfld_ );

    histeqfld_ = new uiGenInput( this, tr("Use Histogram Equalisation"),
				 BoolInpSpec(false) );
    histeqfld_->attach( alignedBelow, usemodefld_ );

    uiSlider::Setup slsu; slsu.withedit( true );
    nrsegsfld_ = new uiSlider( this, slsu );
    nrsegsfld_->setInterval( StepInterval<int>(1,20,1) );
    nrsegsfld_->setTickMarks( uiSlider::Below );
    nrsegsfld_->attach( alignedBelow, histeqfld_ );
    dosegbox_ = new uiCheckBox( this, tr("Segmentize") );
    dosegbox_->attach( leftOf, nrsegsfld_ );

    mAttachCB( postFinalise(), uiEdMapperSetupDlg::initFldsCB );
}


bool isFixed() const
{
    return rangefld_->isChecked();
}

void initFldsCB( CallBacker* )
{
    handleMapperSetupChange();
    int nrsegs = setup().nrSegs();
    if ( nrsegs < 0 )
	nrsegsfld_->setValue( 5 );

    mAttachCB( rangefld_->checked, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( rangefld_->valuechanged, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( rangefld_->updateRequested, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( clipfld_->updateRequested, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( usemodefld_->modeChange, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( histeqfld_->valuechanged, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( dosegbox_->activated, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( nrsegsfld_->valueChanged, uiEdMapperSetupDlg::updReqCB );
}

void fldSelCB( CallBacker* )
{
    updateFldStates();
}

void updReqCB( CallBacker* )
{
    getFromScreen();
}

void putToScreen()
{
    const bool fixedrange = setup().isFixed();
    rangefld_->setValue( setup().range() );
    rangefld_->setChecked( fixedrange );

    usemodefld_->setMode( setup().seqUseMode() );
    histeqfld_->setValue( setup().doHistEq() );

    ColTab::ClipRatePair clipperc( setup().clipRate() );
    ColTab::convToPerc( clipperc );
    if ( dispbothclips_ )
	clipfld_->setValues( clipperc.first(), clipperc.second() );
    else
	clipfld_->setValue( (clipperc.first()+clipperc.second()) * 0.5 );

    int nrsegs = setup().nrSegs();
    const bool havesegs = nrsegs > 0;
    if ( !havesegs )
	nrsegs = 5;
    else if ( nrsegs > 25 )
	nrsegs = 25;
    nrsegsfld_->setValue( nrsegs );
    dosegbox_->setChecked( havesegs );
}

void getFromScreen()
{
    RefMan<ColTab::MapperSetup> newms = new ColTab::MapperSetup( setup() );

    const bool isfixed = isFixed();
    newms->setSeqUseMode( usemodefld_->mode() );
    newms->setDoHistEq( histeqfld_->getBoolValue() );
    if ( isfixed )
	newms->setFixedRange( rangefld_->getFInterval() );
    else
    {
	newms->setNotFixed();
	ColTab::ClipRatePair cliprate;
	cliprate.first() = fabs( clipfld_->getFValue(0) * 0.01f );
	if ( dispbothclips_ )
	    cliprate.second() = fabs( clipfld_->getFValue(1) * 0.01f );
	else
	    cliprate.second() = cliprate.first();
	newms->setClipRate( cliprate );
    }

    int nrsegs = -1;
    if ( dosegbox_->isChecked() )
	nrsegs = nrsegsfld_->getIntValue();
    newms->setNrSegs( nrsegs );

    setup() = *newms;
}


void updateFldStates()
{
    const bool isfixed = isFixed();
    clipfld_->display( !isfixed );
    nrsegsfld_->setSensitive( dosegbox_->isChecked() );
    setSaveButtonChecked( false );
    setButtonSensitive( SAVE, !isfixed );
}

void handleMapperSetupChange()
{
    putToScreen();
    updateFldStates();
}

bool acceptOK()
{
    getFromScreen();

    if ( !isFixed() && saveButtonChecked() )
	ColTab::setMapperDefaults( setup().clipRate(), setup().doHistEq() );

    seltool_.refreshReq.trigger( this );
    return false;
}

protected:

    uiColTabSelTool&		    seltool_;
    ColTab::MapperSetup&	    setup()
				    { return seltool_.mapper_->setup(); }
    bool		dispbothclips_;

    uiGenInput*		rangefld_;
    uiGenInput*		clipfld_;
    uiGenInput*		histeqfld_;
    uiColSeqUseModeSel*	usemodefld_;
    uiCheckBox*		dosegbox_;
    uiSlider*		nrsegsfld_;

};


class uiColTabSelToolHelper
{
public:

    typedef ColTab::Mapper::DistribType	DistribType;
    typedef Interval<float>		RangeType;

uiColTabSelToolHelper( uiColTabSelTool& st )
    : seltool_(st)
{
}

    uiColTabSelTool&	seltool_;

    uiParent*		parent()
			{ return seltool_.asParent(); }
    ColTab::MapperSetup& setup()
			{ return seltool_.mapper_->setup(); }
    DistribType&	distrib()
			{ return seltool_.mapper_->distribution(); }
    bool		isHor() const
			{ return seltool_.orientation() == OD::Horizontal; }

};


class uiManipMapper : public uiGraphicsView
		    , public uiColTabSelToolHelper
{ mODTextTranslationClass("uiManipMapper");
public:

uiManipMapper( uiColTabSelTool& seltool )
    : uiGraphicsView(seltool.getParent(),"Mapper Manipulator")
    , uiColTabSelToolHelper(seltool)
    , eddlg_(0)
    , meh_(getMouseEventHandler())
    , keh_(getKeyboardEventHandler())
    , distribitem_(0)
    , borderitm_(0)
    , zeroitem_(0)
    , rgstartitm_(0)
    , rgstopitm_(0)
    , movingside_(0)
    , lastmovedside_(0)
    , lastmovepos_(mUdf(float))
{
    setNoBackGround();
    disableScrollZoom();

    const int tbsz = toolButtonSize();
    const int displen = 120;
    const OD::Orientation orient = seltool.orientation();
    const bool ishor = orient == OD::Horizontal;
    setViewWidth( ishor ? displen : tbsz );
    setViewHeight( ishor ? tbsz : displen );
    setStretch( ishor ? 1 : 0, ishor ? 0 : 1 );

    mAttachCB( postFinalise(), uiManipMapper::initCB );
}

~uiManipMapper()
{
    detachAllNotifiers();
}

void initCB( CallBacker* )
{
    mAttachCB( meh_.buttonPressed, uiManipMapper::mousePressCB );
    mAttachCB( meh_.buttonReleased, uiManipMapper::mouseReleaseCB );
    mAttachCB( meh_.movement, uiManipMapper::mouseMoveCB );

    mAttachCB( meh_.doubleClick, uiManipMapper::setupDlgReqCB );
    mAttachCB( keh_.keyReleased, uiManipMapper::keyReleasedCB );

    mAttachCB( reSize, uiManipMapper::reSizeCB );

    reDraw();
}

void mousePressCB( CallBacker* ) { handleMouseBut( true ); }
void mouseReleaseCB( CallBacker* ) { handleMouseBut( false ); }

int longNrPix() const
{
    return xIsLong() ? scene().nrPixX() : scene().nrPixY();
}

int pix4Val( float val ) const
{
    const float relpos = (val - longrg_.start) / longrg_.width();
    const float fpix = relpos * longNrPix();
    return (int)(fpix + 0.5f);
}

float val4Pix( int pix ) const
{
    const float relpos = ((float)pix) / longNrPix();
    return longrg_.start + relpos * longrg_.width();
}

void handleMouseBut( bool ispressed )
{
    movingside_ = 0;
    if ( meh_.isHandled() )
	return;

    const MouseEvent& event = meh_.event();
    if ( event.isWithKey() )
	return;

    if ( ispressed )
    {
	if ( event.rightButton() )
	{
	    doMenu();
	    meh_.setHandled( true );
	    return;
	}
	else if ( event.leftButton() )
	{
	    const RangeType rg = setup().range();
	    const int pix = xIsLong() ? event.x() : event.y();
	    const int nrsnappixs = 8;
	    const int startdist = std::abs( pix-pix4Val(rg.start) );
	    const int stopdist = std::abs( pix-pix4Val(rg.stop) );
	    if ( startdist <= stopdist && startdist < nrsnappixs )
		{ movingside_ = -1; lastmovepos_ = val4Pix(pix); }
	    else if ( stopdist < startdist && stopdist < nrsnappixs )
		{ movingside_ = 1; lastmovepos_ = val4Pix(pix); }
	}
    }

    if ( movingside_ )
	lastmovedside_ = movingside_;
}

bool isZeroMirrored( const RangeType& rg ) const
{
    return ColTab::Mapper::isNearZeroSymmetry( rg );
}

void mouseMoveCB( CallBacker* )
{
    if ( !movingside_ || meh_.isHandled() )
	return;
    const MouseEvent& event = meh_.event();
    if ( event.isWithKey() )
	{ pErrMsg("Huh"); return; }

    int pix = xIsLong() ? event.x() : event.y();
    RangeType newrg = setup().range();
    const bool iszeromirrored = isZeroMirrored( newrg );
    if ( movingside_ < 0 )
    {
	const int othsidepix = pix4Val( newrg.stop );
	if ( othsidepix - pix < 1 )
	    pix = othsidepix - 1;
	newrg.start = val4Pix( pix );
	if ( iszeromirrored )
	    newrg.stop = -newrg.start;
    }
    else
    {
	const int othsidepix = pix4Val( newrg.start );
	if ( pix - othsidepix < 1 )
	    pix = othsidepix + 1;
	newrg.stop = val4Pix( pix );
	if ( iszeromirrored )
	    newrg.start = -newrg.stop;
    }

    if ( iszeromirrored )
	newrg.shift( -newrg.center() );
    setup().setFixedRange( newrg );
}

void keyReleasedCB( CallBacker* )
{
    if ( keh_.isHandled() )
	return;
    const KeyboardEvent& event = keh_.event();
    if ( event.modifier_ != OD::NoButton )
	return;

    if ( event.key_ == OD::KB_Enter || event.key_ == OD::KB_Return )
	setupDlgReqCB( 0 );
    else if ( xIsLong()
	   && (event.key_==OD::KB_Left || event.key_==OD::KB_Right) )
	moveLastUsed( event.key_ == OD::KB_Right );
    else if ( !xIsLong()
	   && (event.key_==OD::KB_Up || event.key_==OD::KB_Down) )
	moveLastUsed( event.key_ == OD::KB_Down );
}

void moveLastUsed( bool incr )
{
    if ( !lastmovedside_ || mIsUdf(lastmovepos_) )
	return;

    RangeType newrg = setup().range();
    const bool iszeromirrored = isZeroMirrored( newrg );
    float& mvval = lastmovedside_ < 0 ? newrg.start : newrg.stop;
    const int toadd = incr ? 1 : -1;
    mvval = val4Pix( pix4Val(mvval) + toadd );
    if ( iszeromirrored )
    {
	float& othval = lastmovedside_ > 0 ? newrg.start : newrg.stop;
	othval = val4Pix( pix4Val(othval) - toadd );
	newrg.shift( -newrg.center() );
    }

    setup().setFixedRange( newrg );
}

void addObjectsToToolBar( uiToolBar& tbar )
{
    tbar.addObject( this, seltool_.maxElemLongDimSize() );
}

void orientationChanged()
{
    reDraw();
}

void handleMapperSetupChange()
{
    if ( eddlg_ )
	eddlg_->handleMapperSetupChange();
    reDraw();
}

void handleDistribChange()
{
    reDraw();
}

void doMenu()
{
    uiMenu* mnu = new uiMenu( parent(), uiStrings::sAction() );
    mnu->insertAction( new uiAction(tr("Re-scale now"),
			mCB(this,uiManipMapper,reScaleReqCB)), 0 );
    mnu->insertAction( new uiAction(m3Dots(tr("Full Edit")),
			mCB(this,uiManipMapper,setupDlgReqCB)), 1 );
    mnu->insertAction( new uiAction(m3Dots(uiStrings::sSettings()),
			mCB(this,uiManipMapper,doSettings)), 2 );
    seltool_.mapperMenuReq.trigger( mnu );
    mnu->exec();
}

void reScaleReqCB( CallBacker* )
{
    setup().setNotFixed();
    setup().sendEntireObjectChangeNotification();
}

void setupDlgReqCB( CallBacker* )
{
    if ( !eddlg_ )
    {
	eddlg_ = new uiEdMapperSetupDlg( seltool_ );
	mAttachCB( eddlg_->windowClosed, uiManipMapper::setupDlgCloseCB );
	eddlg_->show();
    }
    eddlg_->raise();
}

void doSettings( CallBacker* )
{
    uiSettingsDlg dlg( seltool_.asParent(), sColTabSettFactKy );
    dlg.go();
}

void setupDlgCloseCB( CallBacker* )
{
    eddlg_ = 0;
}

void reSizeCB( CallBacker* )
{
    reDraw();
}

bool calcScale()
{
    RefMan<DistribType> drawdistr = distrib().clone();
    DataDistributionChanger<float>(*drawdistr).deSpike();
    DataDistributionInfoExtracter<float>(*drawdistr)
			    .getCurve( longvals_, shortvals_, true );
    drawrg_ = setup().range();

    longrg_ = drawrg_;
    const bool emptydistrib = longvals_.isEmpty();
    const bool emptyrange = mIsUdf(longrg_.start) || mIsUdf(longrg_.start);
    if ( emptyrange && emptydistrib )
	return false;

    if ( !emptydistrib )
    {
	RangeType drg( longvals_.first(), longvals_.last() );
	if ( emptyrange )
	    longrg_ = drg;
	else
	    longrg_.include( drg );
	shortrg_.start = shortrg_.stop = 0.f;
	for ( int idx=0; idx<shortvals_.size(); idx++ )
	    shortrg_.include( shortvals_[idx] );
    }
    else // no distrib, but we have a range
    {
	longvals_.erase(); shortvals_.erase();
	longvals_ += longrg_.start; longvals_ += longrg_.stop;
	shortvals_ += 0.5f; shortvals_ += 0.5f;
	shortrg_ = RangeType( 0.f, 1.f );
    }

    return true;
}

void drawDistrib()
{
    const int xmaxpix = scene().nrPixX() - 1;
    const int ymaxpix = scene().nrPixY() - 1;
    TypeSet<uiPoint> pts;
    const float longwdth = longrg_.width();
    const float shortwdth = shortrg_.width();
    const bool xislong = xIsLong();
    const int sz = longvals_.size();
    for ( int idx=-1; idx<=sz; idx++ )
    {
	float lpos = idx<0 ? longvals_[0]
		   : (idx==sz ? longvals_[sz-1] : longvals_[idx]);
	float spos = shortvals_.validIdx(idx) ? shortvals_[idx]
					      : shortrg_.start;
	const float fxpix = xislong
		? ((lpos - longrg_.start) / longwdth) * xmaxpix
		: ((spos - shortrg_.start) / shortwdth) * xmaxpix;
	const float fypix = xislong
		? ((shortrg_.stop - spos) / shortwdth) * ymaxpix
		: ((lpos - longrg_.start) / longwdth) * ymaxpix;
	pts += uiPoint( mNINT32(fxpix), mNINT32(fypix) );
    }

    if ( longrg_.includes(0.f,false) )
    {
	const float longrelpos = -longrg_.start / longwdth;
	if ( xislong )
	{
	    const float fxpix = longrelpos * xmaxpix;
	    const int xpix = mNINT32( fxpix );
	    zeroitem_ = new uiLineItem( xpix, 0, xpix, ymaxpix );
	}
	else
	{
	    const float fypix = longrelpos * ymaxpix;
	    const int ypix = mNINT32( fypix );
	    zeroitem_ = new uiLineItem( 0, ypix, xmaxpix, ypix );
	}
	scene().addItem( zeroitem_ );
	zeroitem_->setPenColor( Color::Black() );
	zeroitem_->setZValue( 1000 );
    }

    distribitem_ = scene().addPolygon( pts, true );
    distribitem_->setPenColor( Color(0,127,0) );
    distribitem_->setFillColor( Color::DgbColor() );
}


void drawRange()
{
    if ( mIsUdf(drawrg_.start) || mIsUdf(drawrg_.stop) )
	return;

    const int xmaxpix = scene().nrPixX() - 1;
    const int ymaxpix = scene().nrPixY() - 1;
    const float longwdth = longrg_.width();
    const bool xislong = xIsLong();

    uiManipHandleItem::Setup msu;
    msu.color_ = Color( 100, 100, 180 );
    rgstartitm_ = scene().addItem( new uiManipHandleItem(msu,!isHor()) );
    rgstopitm_ = scene().addItem( new uiManipHandleItem(msu,!isHor()) );

    const int nrpixlong = xislong ? xmaxpix : ymaxpix;
    float fpos = nrpixlong * (drawrg_.start-longrg_.start) / longwdth;
    rgstartitm_->setPixPos( fpos );
    fpos = nrpixlong * (drawrg_.stop-longrg_.start) / longwdth;
    rgstopitm_->setPixPos( fpos );
}


void drawBorder()
{
    borderitm_ = scene().addItem(
		    new uiRectItem(0,0,scene().nrPixX()-1,scene().nrPixY()-1) );
    borderitm_->setPenColor( Color::Black() );
    borderitm_->setFillColor( Color::NoColor() );
    borderitm_->setZValue( 99999 );
}


void eraseAll()
{
    delete distribitem_; distribitem_ = 0;
    delete zeroitem_; zeroitem_ = 0;
    delete rgstartitm_; rgstartitm_ = 0;
    delete rgstopitm_; rgstopitm_ = 0;
    delete borderitm_; borderitm_ = 0;
}

void reDraw()
{
    eraseAll();
    if ( calcScale() )
    {
	drawDistrib();
	drawRange();
	drawBorder();
    }
}

    uiEdMapperSetupDlg*	eddlg_;
    MouseEventHandler&	meh_;
    KeyboardEventHandler& keh_;
    RangeType		drawrg_;
    RangeType		longrg_, shortrg_;
    TypeSet<float>	longvals_, shortvals_;
    uiPolygonItem*	distribitem_;
    uiLineItem*		zeroitem_;
    uiManipHandleItem*	rgstartitm_;
    uiManipHandleItem*	rgstopitm_;
    uiRectItem*		borderitm_;

    int			movingside_;
    float		lastmovepos_;
    int			lastmovedside_;

    uiParent*		parent()
			{ return seltool_.asParent(); }
    ColTab::MapperSetup& setup()
			{ return seltool_.mapper_->setup(); }
    DistribType&	distrib()
			{ return seltool_.mapper_->distribution(); }

    inline bool		xIsLong() const		{ return isHor(); }
    bool		isHor() const
			{ return seltool_.orientation() == OD::Horizontal; }

};


class uiMapperScaleTextInput : public CallBacker, public uiColTabSelToolHelper
{ mODTextTranslationClass("uiMapperScaleTextInput");
public:

uiMapperScaleTextInput( uiColTabSelTool& st )
    : uiColTabSelToolHelper(st)
{
    minfld_ = new uiLineEdit( st.getParent(), FloatInpSpec(), "Minimum" );
    minfld_->setToolTip( tr("Mimimum of the scaling range") );
    maxfld_ = new uiLineEdit( st.getParent(), FloatInpSpec(), "Maximum" );
    maxfld_->setToolTip( tr("Maximum of the scaling range") );

    mAttachCB( minfld_->returnPressed, uiMapperScaleTextInput::usrCommitCB );
    mAttachCB( maxfld_->returnPressed, uiMapperScaleTextInput::usrCommitCB );
}

~uiMapperScaleTextInput()
{
    detachAllNotifiers();
}

void usrCommitCB( CallBacker* )
{
    RangeType newrg( minfld_->getFValue(), maxfld_->getFValue() );

    if ( mIsUdf(newrg.start) || mIsUdf(newrg.stop) )
    {
	const RangeType oldrg = setup().range();
	if ( mIsUdf(newrg.start) )
	{
	    newrg.start = oldrg.start;
	    minfld_->setValue( newrg.start );
	}
	if ( mIsUdf(newrg.stop) )
	{
	    newrg.stop = oldrg.stop;
	    maxfld_->setValue( newrg.stop );
	}
    }

    setup().setFixedRange( newrg );
}

void orientationChanged()
{
    // nothing we can do
}

void handleMapperSetupChange()
{
    const RangeType rg = setup().range();
    minfld_->setValue( rg.start );
    maxfld_->setValue( rg.stop );
}

void doInternalLayout( uiObject* wraparound, const ConstraintType attborder )
{
    minfld_->attach( attborder );
    const ConstraintType atttyp = isHor() ? rightOf : alignedBelow;
    if ( !wraparound )
	maxfld_->attach( atttyp, minfld_ );
    else
    {
	wraparound->attach( atttyp, minfld_ );
	maxfld_->attach( atttyp, wraparound );
    }
}

    uiLineEdit*	minfld_;
    uiLineEdit*	maxfld_;

};


uiColTabSelTool::uiColTabSelTool()
    : mapper_(new Mapper)
    , txtscalefld_(0)
    , usemodesel_(0)
    , histeqbut_(0)
    , mapperMenuReq(this)
    , mappingChanged(this)
{
}


uiColTabSelTool::~uiColTabSelTool()
{
}


void uiColTabSelTool::initialise( OD::Orientation orient )
{
    uiColSeqSelTool::initialise( orient );

    if ( Settings::common().isTrue(sKeyShowTextManipulators()) )
	txtscalefld_ = new uiMapperScaleTextInput( *this );

    if ( !Settings::common().isFalse(sKeyShowUseModeSel()) )
	usemodesel_ = new uiColSeqUseModeSel( getParent(), true,
					      uiString::empty() );

    manip_ = new uiManipMapper( *this );

    if ( !Settings::common().isFalse(sKeyShowHistEqBut()) )
    {
	histeqbut_ = new uiToolButton( getParent(), "nohisteq",
				tr("Toggle using histogram equalisation"),
				mCB(this,uiColTabSelTool,histeqButChgCB) );
	histeqbut_->setToggleButton( true );
    }

    if ( isGroup() )
    {
	uiObject* lastobj = disp_;
	const ConstraintType attprev = orient == OD::Horizontal
				    ? rightOf : ensureBelow;
	const ConstraintType attborder = orient == OD::Horizontal
				    ? topBorder : leftBorder;
	if ( txtscalefld_ )
	{
	    txtscalefld_->doInternalLayout( disp_, attborder );
	    lastobj = txtscalefld_->maxfld_;
	}
	if ( usemodesel_ )
	{
	    usemodesel_->attach( attprev, lastobj );
	    usemodesel_->attach( attborder );
	    lastobj = usemodesel_->attachObj();
	}
	manip_->attach( attprev, lastobj );
	manip_->attach( attborder );
	if ( histeqbut_ )
	{
	    histeqbut_->attach( attborder );
	    histeqbut_->attach( rightOf, manip_ );
	}
    }

    mAttachCB( mapper_->setup().objectChanged(),
				uiColTabSelTool::mapSetupChgCB );
    mAttachCB( mapper_->setup().rangeCalculated,
				uiColTabSelTool::mapRangeChgCB );
    mAttachCB( mapper_->distribution().objectChanged(),
				uiColTabSelTool::distribChgCB );
    if ( usemodesel_ )
	mAttachCB( usemodesel_->modeChange, uiColTabSelTool::modeSelChgCB );

    disp_->setMapper( mapper_ );
}


void uiColTabSelTool::addObjectsToToolBar( uiToolBar& tbar )
{
    if ( txtscalefld_ )
	tbar.addObject( txtscalefld_->minfld_, 2 );
    uiColSeqSelTool::addObjectsToToolBar( tbar );
    if ( txtscalefld_ )
	tbar.addObject( txtscalefld_->maxfld_, 3 );
    if ( usemodesel_ )
	usemodesel_->addObjectsToToolBar( tbar );
    manip_->addObjectsToToolBar( tbar );
    if ( histeqbut_ )
	tbar.add( histeqbut_ );
}


void uiColTabSelTool::orientationChanged()
{
    uiColSeqSelTool::orientationChanged();
    manip_->orientationChanged();
    if ( txtscalefld_ )
	txtscalefld_->orientationChanged();
}


void uiColTabSelTool::setMapper( Mapper& mpr )
{
    Mapper* oldmpr = mapper_.ptr();
    const bool isnew = oldmpr != &mpr;
    if ( !isnew )
	return;

    const bool datadiffers = replaceMonitoredRef( mapper_, mpr, 0 );
    if ( oldmpr )
	oldmpr->transferSubObjNotifsTo( mpr );
    disp_->setMapper( &mpr );

    if ( datadiffers )
    {
	handleMapperSetupChange();
	handleDistribChange();
    }
}


void uiColTabSelTool::setRange( Interval<float> rg )
{
    mapper_->setup().setFixedRange( rg );
}


void uiColTabSelTool::modeSelChgCB( CallBacker* )
{
    if ( usemodesel_ )
	mapper_->setup().setSeqUseMode( usemodesel_->mode() );
}


void uiColTabSelTool::histeqButChgCB( CallBacker* )
{
    if ( histeqbut_ )
    {
	const bool dohisteq = histeqbut_->isOn();
	mapper_->setup().setDoHistEq( dohisteq );
	histeqbut_->setIcon( dohisteq ? "histeq" : "nohisteq" );
    }
}


void uiColTabSelTool::mapRangeChgCB( CallBacker* )
{
    manip_->handleMapperSetupChange();
    if ( txtscalefld_ )
	txtscalefld_->handleMapperSetupChange();
}

void uiColTabSelTool::mapSetupChgCB( CallBacker* )
{
    handleMapperSetupChange();
}


void uiColTabSelTool::handleMapperSetupChange()
{
    if ( txtscalefld_ )
	txtscalefld_->handleMapperSetupChange();
    if ( usemodesel_ )
	usemodesel_->setMode( mapper_->setup().seqUseMode() );
    manip_->handleMapperSetupChange();
    if ( histeqbut_ )
	histeqbut_->setOn( mapper_->setup().doHistEq() );
    mappingChanged.trigger();
}


void uiColTabSelTool::handleDistribChange()
{
    manip_->handleDistribChange();
    mappingChanged.trigger();
}


void uiColTabSelTool::distribChgCB( CallBacker* )
{
    handleDistribChange();
}


uiColTabSel::uiColTabSel( uiParent* p, OD::Orientation orient,
				    uiString lbltxt )
    : uiGroup(p,"Color Table Selector")
{
    initialise( orient );

    lbl_ = new uiLabel( this, lbltxt );
    lbl_->attach( leftOf, disp_ );

    setHAlignObj( disp_ );
}


void uiColTabSel::setLabelText( const uiString& txt )
{
    lbl_->setText( txt );
}


mImpluiColSeqSelToolBarTool( uiColTabToolBar, uiColTabSelTool )

uiColTabToolBar::uiColTabToolBar( uiParent* p )
    : uiToolBar(p,tr("Color Table Manipulator"))
    , seltool_(*new uiColTabToolBarTool(this))
{
}


class uiColTabSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiColTabSettingsGroup);
public:

    mDecluiSettingsGroupPublicFns( uiColTabSettingsGroup,
				   LooknFeel, sColTabSettFactKy, "colorbar",
				   uiStrings::sColorTable(), mTODOHelpKey )

uiColTabSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,setts)
#define mCTSettsVal( fn, ky ) setts.fn(uiColTabSelTool::ky())
    , initialshowtxtmanip_(mCTSettsVal(isTrue,sKeyShowTextManipulators))
    , initialshowusemodesel_(!mCTSettsVal(isFalse,sKeyShowUseModeSel))
    , initialshowhisteq_(!mCTSettsVal(isFalse,sKeyShowHistEqBut))
    , initialasymclip_(mCTSettsVal(isTrue,sKeyEnableAsymmetricClipping))
{
#   define mCTInpFld( str, var ) \
    new uiGenInput( this, str, BoolInpSpec(var) )
    txtmanipfld_ = mCTInpFld(
			  tr("Enable textual range fields"),
			  initialshowtxtmanip_ );
    usemodefld_ = mCTInpFld( tr("Show Flip/Cyclic shortcut tool"),
			  initialshowusemodesel_ );
    usemodefld_->attach( alignedBelow, txtmanipfld_ );
    histeqfld_ = mCTInpFld( tr("Show Histogram Equalisation button"),
			  initialshowhisteq_ );
    histeqfld_->attach( alignedBelow, usemodefld_ );
    asymfld_ = mCTInpFld( tr("Enable seting asymmetric clipping"),
			  initialasymclip_ );
    asymfld_->attach( alignedBelow, histeqfld_ );

    bottomobj_ = asymfld_;
}


void doCommit( uiRetVal& )
{
    updateSettings( initialshowtxtmanip_, txtmanipfld_->getBoolValue(),
		    uiColTabSelTool::sKeyShowTextManipulators() );
    updateSettings( initialshowusemodesel_, usemodefld_->getBoolValue(),
		    uiColTabSelTool::sKeyShowUseModeSel() );
    updateSettings( initialshowhisteq_, histeqfld_->getBoolValue(),
		    uiColTabSelTool::sKeyShowHistEqBut() );
    if ( changed_ )
	needsrestart_ = true;

    updateSettings( initialasymclip_, asymfld_->getBoolValue(),
		    uiColTabSelTool::sKeyEnableAsymmetricClipping() );
}

    const bool	initialshowtxtmanip_;
    const bool	initialshowusemodesel_;
    const bool	initialshowhisteq_;
    const bool	initialasymclip_;

    uiGenInput*	txtmanipfld_;
    uiGenInput*	usemodefld_;
    uiGenInput*	histeqfld_;
    uiGenInput*	asymfld_;

};


void uiColTabSelTool::initClass()
{
    uiColTabSettingsGroup::initClass();
}

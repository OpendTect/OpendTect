/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicoltabsel.h"
#include "uicolseqdisp.h"
#include "uicolseqman.h"
#include "coltabmapper.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uigraphicsview.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uifunctiondisplay.h"

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
    showAlwaysOnTop();
    setDeleteOnClose( true );
    setOkCancelText( uiStrings::sApply(), uiStrings::sClose() );

    rangefld_ = new uiGenInput( this, tr("Fixed scale range"),
				      FloatInpIntervalSpec() );
    rangefld_->setWithCheck( true );

    usemodefld_ = new uiColSeqUseMode( this );
    usemodefld_->attach( alignedBelow, rangefld_ );

    clipfld_ = new uiGenInput( this, tr("Percentage clipped"),
			      FloatInpIntervalSpec() );
    clipfld_->setElemSzPol( uiObject::Small );
    clipfld_->attach( alignedBelow, usemodefld_ );

    skipsymscanfld_ = new uiGenInput( this, tr("Skip data symmetry scan"),
	     BoolInpSpec(true,tr("Symmetry"), tr("No symmetry")) );
    skipsymscanfld_->setWithCheck( true );
    skipsymscanfld_->attach( alignedBelow, clipfld_ );

    midvalfld_ = new uiGenInput( this, tr("Symmetry is around"),FloatInpSpec());
    midvalfld_->attach( alignedBelow, skipsymscanfld_ );

    mAttachCB( postFinalise(), uiEdMapperSetupDlg::initFldsCB );
}


bool isFixed() const
{
    return rangefld_->isChecked();
}

void initFldsCB( CallBacker* )
{
    handleMapperSetupChange();

    mAttachCB( rangefld_->checked, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( rangefld_->valuechanged, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( skipsymscanfld_->checked, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( skipsymscanfld_->valuechanged, uiEdMapperSetupDlg::fldSelCB );
    mAttachCB( rangefld_->updateRequested, uiEdMapperSetupDlg::updReqCB );
    mAttachCB( clipfld_->updateRequested, uiEdMapperSetupDlg::updReqCB );
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

    Interval<float> clipperc( setup().clipRate() );
    clipperc.scale( 100.f );
    clipfld_->setValue( clipperc );

    skipsymscanfld_->setChecked( !setup().guessSymmetry() );
    const float symmidval = setup().symMidVal();
    const bool knownsymm = !mIsUdf(symmidval);
    skipsymscanfld_->setValue( knownsymm );
    midvalfld_->setValue( knownsymm ? symmidval : 0.f );
}

void getFromScreen()
{
    RefMan<ColTab::MapperSetup> newms = new ColTab::MapperSetup( setup() );

    const bool isfixed = isFixed();
    newms->setIsFixed( isfixed );
    newms->setSeqUseMode( usemodefld_->mode() );
    if ( isfixed )
	newms->setRange( rangefld_->getFInterval() );
    else
    {
	Interval<float> cliprate = clipfld_->getFInterval();
	cliprate.start = fabs( cliprate.start * 0.01f );
	cliprate.stop = fabs( cliprate.stop * 0.01f );
	newms->setClipRate( cliprate );
	const bool guesssymm = !skipsymscanfld_->isChecked();
	newms->setGuessSymmetry( guesssymm );
	if ( !guesssymm )
	    newms->setSymMidVal( skipsymscanfld_->getBoolValue()
		    ? midvalfld_->getFValue() : mUdf(float) );
    }

    setup() = *newms;
}


void updateFldStates()
{
    const bool isfixed = isFixed();
    const bool guesssymm = !isfixed && !skipsymscanfld_->isChecked();
    clipfld_->display( !isfixed );
    skipsymscanfld_->display( !isfixed );
    midvalfld_->display( !isfixed && !guesssymm
		    && skipsymscanfld_->getBoolValue() );
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
	ColTab::setMapperDefaults( setup().clipRate(), setup().symMidVal(),
				   setup().guessSymmetry() );

    seltool_.refreshReq.trigger( this );
    return false;
}

protected:

    uiColTabSelTool&		    seltool_;
    ColTab::MapperSetup&	    setup()
				    { return *seltool_.mappersetup_; }
    uiColTabSelTool::DistribType&   distrib()
				    { return *seltool_.distrib_; }

    uiGenInput*		rangefld_;
    uiGenInput*		clipfld_;
    uiGenInput*		skipsymscanfld_;
    uiGenInput*		midvalfld_;
    uiColSeqUseMode*	usemodefld_;

};


class uiManipMapperSetup : public uiGraphicsView
{ mODTextTranslationClass("uiManipMapperSetup");
public:

    typedef DataDistribution<float>	DistribType;

uiManipMapperSetup( uiColTabSelTool& seltool )
    : uiGraphicsView(seltool.asParent(),"Mapper Manipulator")
    , seltool_(seltool)
    , eddlg_(0)
    , meh_(getMouseEventHandler())
    , keh_(getKeyboardEventHandler())
    , distribitem_(0)
    , rgstartitm_(0)
    , rgstopitm_(0)
    , movingside_(0)
{
    disableScrollZoom();
    mAttachCB( postFinalise(), uiManipMapperSetup::initCB );
}

~uiManipMapperSetup()
{
    detachAllNotifiers();
}

void initCB( CallBacker* )
{
    mAttachCB( meh_.buttonPressed, uiManipMapperSetup::mousePressCB );
    mAttachCB( meh_.buttonReleased, uiManipMapperSetup::mouseReleaseCB );
    mAttachCB( meh_.movement, uiManipMapperSetup::mouseMoveCB );

    mAttachCB( meh_.doubleClick, uiManipMapperSetup::setupDlgReqCB );
    mAttachCB( keh_.keyReleased, uiManipMapperSetup::keyReleasedCB );

    mAttachCB( reSize, uiManipMapperSetup::reSizeCB );

    reDraw();
}

void mousePressCB( CallBacker* ) { handleMouseBut( true ); }
void mouseReleaseCB( CallBacker* ) { handleMouseBut( true ); }

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
    if ( meh_.isHandled() )
	return;

    movingside_ = 0;
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
	    const int pix = event.x();
	    const int nrsnappixs = 5;
	    if ( abs(pix-pix4Val(maprg_.start)) < nrsnappixs )
		{ movingside_ = -1; lastmovepos_ = val4Pix(pix); }
	    else if ( abs(pix-pix4Val(maprg_.stop)) < nrsnappixs )
		{ movingside_ = 1; lastmovepos_ = val4Pix(pix); }
	}
    }
}

void mouseMoveCB( CallBacker* )
{
    if ( !movingside_ || meh_.isHandled() )
	return;
    const MouseEvent& event = meh_.event();
    if ( event.isWithKey() )
	{ pErrMsg("Huh"); return; }

    int pix = event.x();
    if ( movingside_ < 0 )
    {
	const int othsidepix = pix4Val( maprg_.stop );
	if ( othsidepix - pix < 1 )
	    pix = othsidepix - 1;
	maprg_.start = val4Pix( pix );
    }
    else
    {
	const int othsidepix = pix4Val( maprg_.start );
	if ( pix - othsidepix < 1 )
	    pix = othsidepix + 1;
	maprg_.stop = val4Pix( pix );
    }

    setup().setRange( maprg_ );
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
}

int maxLongSz() const
{
    return 3 * uiObject::iconSize();
}

void addObjectsToToolBar( uiToolBar& tbar )
{
    setMaximumWidth( maxLongSz() );
    setMaximumHeight( maxLongSz() );
    tbar.addObject( this );
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
    mnu->insertItem( new uiAction(tr("Re-scale now"),
		        mCB(this,uiManipMapperSetup,reScaleReqCB)), 0 );
    mnu->insertItem( new uiAction(m3Dots(tr("Full Edit")),
		        mCB(this,uiManipMapperSetup,setupDlgReqCB)), 1 );
    seltool_.mapperMenuReq.trigger( mnu );
    mnu->exec();
}

void reScaleReqCB( CallBacker* )
{
    const bool wasfixed = setup().isFixed();
    setup().setIsFixed( false );
    if ( wasfixed )
	setup().setIsFixed( true );
    else
	setup().sendEntireObjectChangeNotification();
}

void setupDlgReqCB( CallBacker* )
{
    if ( !eddlg_ )
    {
	eddlg_ = new uiEdMapperSetupDlg( seltool_ );
	mAttachCB( eddlg_->windowClosed, uiManipMapperSetup::setupDlgCloseCB );
	eddlg_->show();
    }
    eddlg_->raise();
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
    distrib().getCurve( longvals_, shortvals_, true );
    longrg_ = setup().range();

    const bool emptydistrib = longvals_.isEmpty();
    const bool emptyrange = mIsUdf(longrg_.start) || mIsUdf(longrg_.start);
    if ( emptyrange && emptydistrib )
	return false;

    if ( !emptydistrib )
    {
	Interval<float> drg( longvals_.first(), longvals_.last() );
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
	shortrg_ = Interval<float>( 0.f, 1.f );
    }

    maprg_ = longrg_;
    longrg_.widen( longrg_.width()/6.f );
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
		: ((longrg_.stop - lpos) / longwdth) * ymaxpix;
	pts += uiPoint( mNINT32(fxpix), mNINT32(fypix) );
    }

    distribitem_ = scene().addPolygon( pts, true );
    distribitem_->setPenColor( Color(0,127,0) );
    distribitem_->setFillColor( Color::DgbColor() );
}


void drawRange()
{
    if ( mIsUdf(maprg_.start) || mIsUdf(maprg_.stop) )
	return;

    const int xmaxpix = scene().nrPixX() - 1;
    const int ymaxpix = scene().nrPixY() - 1;
    const float longwdth = longrg_.width();
    const bool xislong = xIsLong();
    uiManipHandleItem::Setup msu;
    msu.hor_ = !isHor(); msu.thickness_ = 3;
    msu.start_ = 0; msu.stop_ = xislong ? ymaxpix : xmaxpix;
    msu.color_ = Color::Black();

    const int nrpixlong = xislong ? xmaxpix : ymaxpix;
    float fpos = nrpixlong * (maprg_.start-longrg_.start) / longwdth;
    rgstartitm_ = scene().addItem( new uiManipHandleItem(msu,fpos) );
    fpos = nrpixlong * (maprg_.stop-longrg_.start) / longwdth;
    rgstopitm_ = scene().addItem( new uiManipHandleItem(msu,fpos) );
}


void eraseAll()
{
    delete distribitem_; distribitem_ = 0;
    delete rgstartitm_; rgstartitm_ = 0;
    delete rgstopitm_; rgstopitm_ = 0;
}

void reDraw()
{
    eraseAll();
    if ( calcScale() )
    {
	drawDistrib();
	drawRange();
    }
}

    uiColTabSelTool&	seltool_;
    uiEdMapperSetupDlg*	eddlg_;
    MouseEventHandler&	meh_;
    KeyboardEventHandler& keh_;
    Interval<float>	maprg_;
    Interval<float>	longrg_, shortrg_;
    TypeSet<float>	longvals_, shortvals_;
    uiPolygonItem*	distribitem_;
    uiManipHandleItem*	rgstartitm_;
    uiManipHandleItem*	rgstopitm_;

    int			movingside_;
    float		lastmovepos_;

    uiParent*			    parent()
				    { return seltool_.asParent(); }
    ColTab::MapperSetup&	    setup()
				    { return *seltool_.mappersetup_; }
    uiColTabSelTool::DistribType&   distrib()
				    { return *seltool_.distrib_; }

    inline bool		xIsLong() const		{ return isHor(); }
    bool		isHor() const
			{ return seltool_.orientation() == OD::Horizontal; }

};


uiColTabSelTool::uiColTabSelTool()
    : distrib_(new DistribType)
    , mappersetup_(new MapperSetup)
    , mapperSetupChanged(this)
    , distributionChanged(this)
    , mapperMenuReq(this)
{
}


uiColTabSelTool::~uiColTabSelTool()
{
}


void uiColTabSelTool::initialise( OD::Orientation orient )
{
    uiColSeqSelTool::initialise( orient );

    manip_ = new uiManipMapperSetup( *this );

    mAttachCB( mappersetup_->objectChanged(), uiColTabSelTool::mapSetupChgCB );
    mAttachCB( newManDlg, uiColTabSelTool::newManDlgCB );
}


void uiColTabSelTool::addObjectsToToolBar( uiToolBar& tbar )
{
    uiColSeqSelTool::addObjectsToToolBar( tbar );
    manip_->addObjectsToToolBar( tbar );
}


void uiColTabSelTool::orientationChanged()
{
    uiColSeqSelTool::orientationChanged();
    manip_->orientationChanged();
}


void uiColTabSelTool::useMapperSetup( const MapperSetup& msu )
{
    if ( replaceMonitoredRef(mappersetup_,const_cast<MapperSetup&>(msu)) )
	handleMapperSetupChange();
}


void uiColTabSelTool::useDistribution( const DistribType& distr )
{
    const bool issame = &distr == distrib_.ptr();
    replaceMonitoredRef( distrib_, const_cast<DistribType&>(distr) );
    if ( !issame )
    {
	if ( mandlg_ )
	    mandlg_->useDistrib( distrib_ );
	manip_->handleDistribChange();
    }
}


void uiColTabSelTool::newManDlgCB( CallBacker* )
{
    if ( mandlg_ )
	mandlg_->useDistrib( distrib_ );
}


void uiColTabSelTool::mapSetupChgCB( CallBacker* )
{
    handleMapperSetupChange();
}


void uiColTabSelTool::handleMapperSetupChange()
{
    setSeqUseMode( mappersetup_->seqUseMode() );
    manip_->handleMapperSetupChange();
    mapperSetupChanged.trigger();
}


void uiColTabSelTool::handleDistribChange()
{
    manip_->handleDistribChange();
    distributionChanged.trigger();
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

    if ( !lbltxt.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, lbltxt );
	lbl->attach( leftOf, disp_ );
    }

    manip_->attach( rightOf, disp_ );
}


void uiColTabSel::setLabelText( const uiString& txt )
{
    lbl_->setText( txt );
}


mImpluiColSeqSelToolBarTool( uiColTabToolBar, uiColTabSelTool )

uiColTabToolBar::uiColTabToolBar( uiParent* p )
    : uiToolBar(p,tr("Color Selection"))
    , seltool_(*new uiColTabToolBarTool(this))
{
}

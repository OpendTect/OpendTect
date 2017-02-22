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

uiManipMapperSetup( uiColTabSelTool& seltool )
    : uiGraphicsView(seltool.asParent(),"Mapper Manipulator")
    , seltool_(seltool)
    , eddlg_(0)
    , meh_(getMouseEventHandler())
    , keh_(getKeyboardEventHandler())
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

void handleMouseBut( bool ispressed )
{
    if ( meh_.isHandled() )
	return;
    const MouseEvent& event = meh_.event();
    if ( event.isWithKey() )
	return;

    if ( event.rightButton() && ispressed )
    {
	doMenu();
	meh_.setHandled( true );
    }
}

void mouseMoveCB( CallBacker* )
{
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

void addObjectsToToolBar( uiToolBar& tbar )
{
    setMaximumWidth( 3*uiObject::iconSize() );
    tbar.addObject( this );
}

void handleMapperSetupChange()
{
    if ( eddlg_ )
	eddlg_->handleMapperSetupChange();
    reDrawRange();
}

void handleDistribChange()
{
    reDrawDistrib();
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

void reDrawDistrib()
{
    /*
    MonitorLock ml( distrib() );
    const int sz = distrib().size();
    if ( sz < 2 )
    {
	if ( polygonitem_ )
	    polygonitem_->setVisible( false );
	return;
    }
    TypeSet<uiPoint> pts;

    if ( !polygonitem_ )
	polygonitem_ = scene().addPolygon( pts, true );
    else
	polygonitem_->setPolygon( pts );
	*/
}


void reDrawRange()
{
}

void reDraw()
{
    reDrawDistrib();
    reDrawRange();
}

    uiColTabSelTool&	seltool_;
    uiEdMapperSetupDlg*	eddlg_;
    MouseEventHandler&	meh_;
    KeyboardEventHandler& keh_;
    Interval<float>	scale_;
    Interval<float>	range4scale_;

    uiParent*			    parent()
				    { return seltool_.asParent(); }
    ColTab::MapperSetup&	    setup()
				    { return *seltool_.mappersetup_; }
    uiColTabSelTool::DistribType&   distrib()
				    { return *seltool_.distrib_; }

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

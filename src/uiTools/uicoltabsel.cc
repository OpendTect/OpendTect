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

static uiColTabSelTool* globseltool_ = 0;
void SetuiCOLTAB( uiColTabSelTool* st )
{
    globseltool_ = st;
}
uiColTabSelTool& uiCOLTAB()
{
    return *globseltool_;
}


class uiColTabScaleDlg : public uiDialog
{ mODTextTranslationClass(uiColTabScaleDlg);
public:

uiColTabScaleDlg( uiColTabSelTool& st )
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

    mAttachCB( postFinalise(), uiColTabScaleDlg::initFldsCB );
}


bool isFixed() const
{
    return rangefld_->isChecked();
}

void initFldsCB( CallBacker* )
{
    handleMapperChange();

    mAttachCB( rangefld_->checked, uiColTabScaleDlg::fldSelCB );
    mAttachCB( rangefld_->valuechanged, uiColTabScaleDlg::fldSelCB );
    mAttachCB( skipsymscanfld_->checked, uiColTabScaleDlg::fldSelCB );
    mAttachCB( skipsymscanfld_->valuechanged, uiColTabScaleDlg::fldSelCB );
}

void fldSelCB( CallBacker* )
{
    updateFldStates();
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

void handleMapperChange()
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

    uiColTabSelTool&	seltool_;
    ColTab::MapperSetup& setup()    { return *seltool_.mappersetup_; }

    uiGenInput*		rangefld_;
    uiGenInput*		clipfld_;
    uiGenInput*		skipsymscanfld_;
    uiGenInput*		midvalfld_;
    uiColSeqUseMode*	usemodefld_;

};



uiColTabSelTool::uiColTabSelTool()
    : distrib_(new DistribType)
    , mappersetup_(new MapperSetup)
    , scaledlg_(0)
    , mapperSetupChanged(this)
    , distributionChanged(this)
{
    usebasicmenu_ = false;
    mAttachCB( menuReq, uiColTabSelTool::menuCB );
    mAttachCB( newManDlg, uiColTabSelTool::newManDlgCB );
}


uiColTabSelTool::~uiColTabSelTool()
{
}


void uiColTabSelTool::initialise( OD::Orientation orient )
{
    uiColSeqSelTool::initialise( orient );

    mAttachCB( mappersetup_->objectChanged(), uiColTabSelTool::mapSetupChgCB );
    mAttachCB( distrib_->objectChanged(), uiColTabSelTool::distribChgCB );
}


void uiColTabSelTool::addObjectsToToolBar( uiToolBar& tbar )
{
    uiColSeqSelTool::addObjectsToToolBar( tbar );
}


void uiColTabSelTool::setMapperSetup( const MapperSetup& msu )
{
    replaceMonitoredRef( mappersetup_, const_cast<MapperSetup&>(msu) );
}


void uiColTabSelTool::setDistribution( const DistribType& distr )
{
    replaceMonitoredRef( distrib_, const_cast<DistribType&>(distr) );
}


void uiColTabSelTool::menuCB( CallBacker* )
{
    PtrMan<uiMenu> mnu = getBasicMenu();
    mnu->insertItem( new uiAction(m3Dots(tr("Ranges/Clipping")),
		        mCB(this,uiColTabSelTool,scaleDlgReqCB)), 2 );
    mnu->exec();
}


void uiColTabSelTool::scaleDlgReqCB( CallBacker* )
{
    if ( !scaledlg_ )
    {
	scaledlg_ = new uiColTabScaleDlg( *this );
	mAttachCB( scaledlg_->windowClosed, uiColTabSelTool::scaleDlgCloseCB );
	scaledlg_->show();
    }
    scaledlg_->raise();
}


void uiColTabSelTool::newManDlgCB( CallBacker* )
{
    mandlg_->setDistrib( distrib_ );
}


void uiColTabSelTool::mapSetupChgCB( CallBacker* )
{
    setSeqUseMode( mappersetup_->seqUseMode() );
    if ( scaledlg_ )
	scaledlg_->handleMapperChange();
    mapperSetupChanged.trigger();
}


void uiColTabSelTool::distribChgCB( CallBacker* )
{
    distributionChanged.trigger();
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

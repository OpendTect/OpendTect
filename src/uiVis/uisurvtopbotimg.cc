/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurvtopbotimg.h"
#include "vistopbotimage.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uislider.h"
#include "vismaterial.h"
#include "vissurvscene.h"

#include "oddirs.h"
#include "od_helpids.h"
#include "settingsaccess.h"
#include "survinfo.h"


class uiSurvTopBotImageGrp : public uiGroup
{ mODTextTranslationClass(uiSurvTopBotImageGrp)
public:

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop,
			const StepInterval<float>& zrng )
    : uiGroup(p, istop ? "Top img grp" : "Bot img grp")
    , dlg_(p)
    , istop_(istop)
    , img_(p->scene_->getTopBotImage(istop))
    , zrng_(zrng)
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    fnmfld_ = new uiFileInput( leftgrp,
	    istop_ ? tr("Top image") : tr("Bottom image"), su);
    fnmfld_->setWithCheck( true );
    fnmfld_->valueChanged.notify( mCB(this,uiSurvTopBotImageGrp,newFile) );
    mAttachCB( fnmfld_->checked,uiSurvTopBotImageGrp::onOff );
    fnmfld_->setChecked( img_ && img_->isOn() );

#define mAddCoordchgCB( notif ) \
     mAttachCB( notif, uiSurvTopBotImageGrp::coordChg );

    uiSlider::Setup slsu( tr("Vertical position (Z)") );
    slsu.withedit( true );
    slsu.isvertical( true );
    slsu.sldrsize( 100 );
    zposfld_ = new uiSlider( this, slsu );
    zposfld_->setInverted( true );
    zposfld_->setScale( zrng_.step, zrng_.start );
    zposfld_->setInterval( zrng_ );
    zposfld_->attach( rightOf, leftgrp );
    zposfld_->setValue( istop_ ? zrng_.start : zrng_.stop );
    mAddCoordchgCB( zposfld_->valueChanged );

    const Coord mincrd = SI().minCoord(true);
    const Coord maxcrd = SI().maxCoord(true);
    tlfld_ = new uiGenInput( leftgrp, tr("NorthWest (TopLeft) Coordinate"),
			     PositionInpSpec(Coord(mincrd.x,maxcrd.y)) );
    tlfld_->setElemSzPol( uiObject::MedVar );
    tlfld_->attach( alignedBelow, fnmfld_ );
    mAddCoordchgCB( tlfld_->valueChanged );

    brfld_ = new uiGenInput( leftgrp, tr("SouthEast (BottomRight) Coordinate"),
			     PositionInpSpec(Coord(maxcrd.x,mincrd.y)) );
    brfld_->setElemSzPol( uiObject::MedVar );
    brfld_->attach( alignedBelow, tlfld_ );
    mAddCoordchgCB( brfld_->valueChanged );

    transpfld_ = new uiSlider( leftgrp,
			       uiSlider::Setup(uiStrings::sTransparency())
						.sldrsize(150).withedit(true),
			       "Transparency slider" );
    transpfld_->attach( alignedBelow, brfld_ );
    transpfld_->setMinValue( 0 );
    transpfld_->setMaxValue( 100 );
    transpfld_->setStep( 1 );
    mAttachCB( transpfld_->valueChanged, uiSurvTopBotImageGrp::transpChg );
    mAttachCB( postFinalize(), uiSurvTopBotImageGrp::finalizedCB );
}

~uiSurvTopBotImageGrp()
{
    detachAllNotifiers();
}


void finalizedCB( CallBacker* )
{
    fillCurrent();
    const int nrdec = SI().nrXYDecimals();
    tlfld_->setNrDecimals( nrdec, 0 );
    tlfld_->setNrDecimals( nrdec, 1 );
    brfld_->setNrDecimals( nrdec, 0 );
    brfld_->setNrDecimals( nrdec, 1 );
}

void fillCurrent()
{
    if ( !img_ )
	return;

    fnmfld_->setChecked( img_->isOn() );
    fnmfld_->setFileName( img_->getImageFilename() );
    tlfld_->setValue( img_->topLeft() );
    brfld_->setValue( img_->bottomRight() );
    transpfld_->setValue( img_->getTransparency()*100 );
    zposfld_->setValue( mCast(float,img_->topLeft().z) );
}

void newFile( CallBacker* )
{
    dlg_->newFile( istop_, fnmfld_->fileName() );
}

void onOff( CallBacker* )
{
    const bool ison = fnmfld_->isChecked();
    if ( !img_ && ison )
    {
	dlg_->scene_->createTopBotImage( istop_ );
	img_ = dlg_->scene_->getTopBotImage( istop_ );
	coordChg( nullptr );
    }

    dlg_->setOn( istop_, ison );
    tlfld_->display( ison );
    brfld_->display( ison );
    transpfld_->display( ison );
    zposfld_->display( ison );
}

void coordChg( CallBacker* )
{
    const Coord3 tlcoord( tlfld_->getCoord(), zposfld_->getFValue() );
    const Coord3 brcoord( brfld_->getCoord(), zposfld_->getFValue() );
    dlg_->setCoord( istop_, tlcoord, brcoord );
}

void transpChg( CallBacker* )
{
    dlg_->setTransparency( istop_,
			   float(transpfld_->getIntValue()/100.) );
}

    const bool		istop_;

    uiSurvTopBotImageDlg* dlg_;
    visBase::TopBotImage* img_;

    uiFileInput*	fnmfld_;
    uiGenInput*		tlfld_;
    uiGenInput*		brfld_;
    uiSlider*		transpfld_;
    uiSlider*		zposfld_;
    const StepInterval<float>&	 zrng_;
};


uiSurvTopBotImageDlg::uiSurvTopBotImageDlg( uiParent* p,
					    visSurvey::Scene* scene )
    : uiDialog(p,uiDialog::Setup(tr("Set Top/Bottom Images"),
			mNoDlgTitle,mODHelpKey(mSurvTopBotImageDlgHelpID)))
    , scene_( scene )
{
    setCtrlStyle( CloseOnly );

    uiLabel* lbl = nullptr;
    if ( !SettingsAccess().doesUserWantShading(false) )
    {
	uiString msg =
		tr("Warning: OpenGL Shading for surface rendering is turned "
		   "off.\nPlease turn on shading in the Look and Feel Settings "
		   "to display these images.\n");
	lbl = new uiLabel( this, msg );
	lbl->attach( leftBorder );
    }

    topfld_ = new uiSurvTopBotImageGrp( this, true,
					scene_->getTrcKeyZSampling().zsamp_ );
    if ( lbl )
	topfld_->attach( ensureBelow, lbl );
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, topfld_ );
    botfld_ = new uiSurvTopBotImageGrp( this, false,
					scene_->getTrcKeyZSampling().zsamp_ );
    botfld_->attach( alignedBelow, topfld_ );
    botfld_->attach( ensureBelow, sep );
}


uiSurvTopBotImageDlg::~uiSurvTopBotImageDlg()
{}


void uiSurvTopBotImageDlg::newFile( bool istop, const char* fnm )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setRGBImageFromFile( fnm );
}


void uiSurvTopBotImageDlg::setOn( bool istop, bool ison )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->turnOn( ison );
}


void uiSurvTopBotImageDlg::setCoord( bool istop, const Coord3& tl,
						 const Coord3& br  )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setPos( tl, br );
}


void uiSurvTopBotImageDlg::setTransparency( bool istop, float val )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setTransparency( val );
}

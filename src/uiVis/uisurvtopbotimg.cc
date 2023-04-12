/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurvtopbotimg.h"
#include "vistopbotimage.h"

#include "uiimagesel.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uislider.h"
#include "vissurvscene.h"

#include "od_helpids.h"
#include "survinfo.h"
#include "settingsaccess.h"


class uiSurvTopBotImageGrp : public uiGroup
{ mODTextTranslationClass(uiSurvTopBotImageGrp)
public:

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop,
			const StepInterval<float>& zrng )
    : uiGroup(p, istop ? "Top img grp" : "Bot img grp")
    , istop_(istop)
    , dlg_(p)
    , img_(p->scene_->getTopBotImage(istop))
    , zrng_(zrng)
{
    uiImageSel::Setup su( istop_ ? tr("Top image") : tr("Bottom image" ) );
    su.filldef(false).optional( true );
    imagefld_ = new uiImageSel( this, true, su );
    imagefld_->setChecked( img_ ? img_->isOn() : false );
    mAttachCB( imagefld_->selectionDone, uiSurvTopBotImageGrp::newImage );
    mAttachCB( imagefld_->optionalChecked, uiSurvTopBotImageGrp::onOff );

    uiSlider::Setup slsu( tr("Vertical position (Z)") );
    slsu.sldrsize(150).withedit( true );
    zposfld_ = new uiSlider( this, slsu, "Z slider" );
    zposfld_->setScale( zrng_.step, zrng_.start );
    zposfld_->setInterval( zrng_ );
    zposfld_->attach( alignedBelow, imagefld_ );
    zposfld_->setValue( istop_ ? zrng_.start : zrng_.stop );
    mAttachCB( zposfld_->valueChanged, uiSurvTopBotImageGrp::coordChg );

    slsu.lbl_ = uiStrings::sTransparency();
    transpfld_ = new uiSlider( this, slsu, "Transparency slider" );
    transpfld_->attach( alignedBelow, zposfld_ );
    transpfld_->setMinValue( 0 );
    transpfld_->setMaxValue( 100 );
    transpfld_->setStep( 1 );
    transpfld_->setNrDecimalsEditFld( 0 );
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
}

void fillCurrent()
{
    if ( !img_ )
	return;

    imagefld_->setChecked( img_->isOn() );
    imagefld_->setInput( img_->getImageID() );
    transpfld_->setValue( img_->getTransparency()*100 );
    zposfld_->setValue( mCast(float,img_->topLeft().z) );
}

void newImage( CallBacker* )
{
    dlg_->newImage( istop_, imagefld_->key() );
}

void onOff( CallBacker* )
{
    const bool ison = imagefld_->isChecked();
    if ( !img_ && ison )
    {
	dlg_->scene_->createTopBotImage( istop_ );
	img_ = dlg_->scene_->getTopBotImage( istop_ );

	const Coord mincrd = SI().minCoord(true);
	const Coord maxcrd = SI().maxCoord(true);
	const double zval = zposfld_->getFValue();
	const Coord3 tlcrd( mincrd.x, maxcrd.y, zval );
	const Coord3 brcrd( maxcrd.x, mincrd.y, zval );
	img_->setPos( tlcrd, brcrd );
    }

    dlg_->setOn( istop_, ison );
    transpfld_->display( ison );
    zposfld_->display( ison );
}

void coordChg( CallBacker* )
{
    dlg_->setZ( istop_, zposfld_->getFValue() );
}

void transpChg( CallBacker* )
{
    dlg_->setTransparency( istop_,
			   float(transpfld_->getIntValue()/100.) );
}

    const bool		istop_;

    uiSurvTopBotImageDlg* dlg_;
    visBase::TopBotImage* img_;

    uiImageSel*		imagefld_;
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


visBase::TopBotImage* uiSurvTopBotImageDlg::getImage( bool istop )
{
    return scene_->getTopBotImage( istop );
}


void uiSurvTopBotImageDlg::newImage( bool istop, const MultiID& mid )
{
    if ( getImage(istop) )
	getImage(istop)->setImageID( mid );
}


void uiSurvTopBotImageDlg::setOn( bool istop, bool ison )
{
    if ( getImage(istop) )
	getImage(istop)->turnOn( ison );
}


void uiSurvTopBotImageDlg::setZ( bool istop, float zval )
{
    auto* image = getImage( istop );
    if ( !image )
	return;

    Coord3 tl = image->topLeft();
    Coord3 br = image->bottomRight();
    tl.z = br.z = zval;
    image->setPos( tl, br );
}


void uiSurvTopBotImageDlg::setTransparency( bool istop, float val )
{
    if ( getImage(istop) )
	getImage(istop)->setTransparency( val );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"
#include "settings.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimarkerstyle.h"
#include "uisellinest.h"
#include "uiseparator.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "visseedpainter.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiMarkerStyleDlg( p, tr("PointSet Display Properties"), true )
    , lsfld_(nullptr)
    , fillcolfld_(nullptr)
    , set_(set)
    , psd_(psd)
{
    uiObject* attachobj = stylefld_->attachObj();
    const bool ispolygon = set.isPolygon();
    if ( ispolygon )
    {
	setCaption( tr("Polygon Display Properties") );

	uiSelLineStyle::Setup stu;
	lsfld_ = new uiSelLineStyle( this, set_.disp_.linestyle_, stu );
	mAttachCB( lsfld_->changed, uiPickPropDlg::linePropertyChanged );
	stylefld_->attach( alignedBelow, lsfld_ );

	uiColorInput::Setup colstu( set_.disp_.fillcolor_ );
	colstu.lbltxt( tr("Fill with") ).withcheck( true )
	      .transp(uiColorInput::Setup::Separate);
	fillcolfld_ = new uiColorInput( this, colstu );
	fillcolfld_->setDoDraw( set_.disp_.dofill_ );
	fillcolfld_->attach( alignedBelow, stylefld_ );
	mAttachCB(fillcolfld_->colorChanged,uiPickPropDlg::fillColorChangeCB);
	mAttachCB(fillcolfld_->doDrawChanged,uiPickPropDlg::fillColorChangeCB);
	attachobj = fillcolfld_->attachObj();
    }

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, attachobj );

    bool usethreshold = true;
    Settings::common().getYN( Pick::Set::sKeyUseThreshold(), usethreshold );
    usethresholdfld_ = new uiCheckBox( this,
		tr("Switch to Point mode for all large PointSets") );
    usethresholdfld_->setChecked( usethreshold );
    mAttachCB( usethresholdfld_->activated, uiPickPropDlg::useThresholdCB );
    usethresholdfld_->attach( alignedBelow, attachobj );
    usethresholdfld_->attach( ensureBelow, sep );

    thresholdfld_ =  new uiGenInput( this, tr("Threshold size for Point mode"));
    thresholdfld_->attach( alignedBelow, usethresholdfld_ );
    mAttachCB( thresholdfld_->valuechanged, uiPickPropDlg::thresholdChangeCB );
    thresholdfld_->setSensitive( usethreshold );
    thresholdfld_->setValue( Pick::Set::getSizeThreshold() );
}


uiPickPropDlg::~uiPickPropDlg()
{
    detachAllNotifiers();
}


void uiPickPropDlg::linePropertyChanged( CallBacker* )
{
    if ( !finalised() || !lsfld_ )
	return;

    set_.disp_.linestyle_ = lsfld_->getStyle();
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::fillColorChangeCB( CallBacker* )
{
    if ( !fillcolfld_ )
	return;

    set_.disp_.fillcolor_ = fillcolfld_->color() ;
    set_.disp_.dofill_ = fillcolfld_->doDraw();

    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    MarkerStyle3D style( (MarkerStyle3D::Type) set_.disp_.markertype_,
	    set_.disp_.pixsize_, set_.disp_.color_ );
    stylefld_->setMarkerStyle( style );
}


void uiPickPropDlg::sliderMove( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );

    if ( set_.disp_.pixsize_ == style.size_ )
	return;

    set_.disp_.pixsize_ = style.size_;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    if ( !finalised() )
	return;

    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    set_.disp_.markertype_ = style.type_;

    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    set_.disp_.color_ = style.color_;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::useThresholdCB( CallBacker* )
{
    const bool usesthreshold = usethresholdfld_->isChecked();
    thresholdfld_->setSensitive( usesthreshold );
    Settings::common().setYN( Pick::Set::sKeyUseThreshold(), usesthreshold );
    Settings::common().write();
}


void uiPickPropDlg::thresholdChangeCB( CallBacker* )
{
    const int thresholdval = thresholdfld_->getIntValue();
    Settings::common().set( Pick::Set::sKeyThresholdSize(), thresholdval );
    Settings::common().write();
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    return set_.writeDisplayPars();
}


uiSeedPainterDlg::uiSeedPainterDlg( uiParent* p,
				    visSurvey::LocationDisplay* psd )
    : uiDialog(p,uiDialog::Setup(tr("PointSet Painter"),mNoDlgTitle,mNoHelpKey)
	    	.modal(false))
    , seedpainter_(psd->getPainter())
{
    setCtrlStyle( CloseOnly );
    setDeleteOnClose( true );
    seedpainter_->ref();
    seedpainter_->activate();

    int maxbrushsize = SI().zRange().nrSteps() / 5;
    if ( maxbrushsize > 1000 ) maxbrushsize = 1000;
    else if ( maxbrushsize > 500 ) maxbrushsize = 500;
    else if ( maxbrushsize > 100 ) maxbrushsize = 100;
    else maxbrushsize = 50;

    auto lbl = new uiLabel( this,
	    tr("Drag to paint seeds\nCtrl-drag to erase") );

    szfld_ = new uiSlider( this, uiSlider::Setup(tr("Brush size"))
	    			 .withedit(true) );
    szfld_->setInterval( StepInterval<int>(1,maxbrushsize,1) );
    szfld_->setValue( seedpainter_->radius() );
    szfld_->sliderMoved.notify( mCB(this,uiSeedPainterDlg,sizeCB) );
    lbl->attach( centeredAbove, szfld_ );

    densfld_ = new uiSlider( this, uiSlider::Setup(tr("Density %"))
	    			   .withedit(true) );
    densfld_->setInterval( StepInterval<int>(1,maxbrushsize,1) );
    densfld_->setValue( seedpainter_->density() );
    densfld_->sliderMoved.notify( mCB(this,uiSeedPainterDlg,densCB) );
    densfld_->attach( alignedBelow, szfld_ );
}


uiSeedPainterDlg::~uiSeedPainterDlg()
{ seedpainter_->deActivate(); seedpainter_->unRef(); }

void uiSeedPainterDlg::sizeCB( CallBacker* )
{
    seedpainter_->setRadius( szfld_->getIntValue() );
}


void uiSeedPainterDlg::densCB( CallBacker* )
{
    seedpainter_->setDensity( densfld_->getIntValue() );
}

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
#include "uimarkerstyle.h"
#include "uisellinest.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiMarkerStyleDlg( p, tr("PointSet Display Properties") )
    , set_( set )
    , psd_( psd )
{
    uiSelLineStyle::Setup stu;
    lsfld_ = new uiSelLineStyle( this, OD::LineStyle(set_.disp_.type_
				,set_.disp_.width_,set_.disp_.color_), stu );
    mAttachCB( lsfld_->changed, uiPickPropDlg::linePropertyChanged );

    stylefld_->attach( alignedBelow, lsfld_ );

    bool usethreshold = true;
    Settings::common().getYN( Pick::Set::sKeyUseThreshold(), usethreshold );
    usethresholdfld_ =
     new uiCheckBox( this, tr("Switch to Point mode for all large PointSets") );
    usethresholdfld_->setChecked( usethreshold );
    mAttachCB( usethresholdfld_->activated, uiPickPropDlg::useThresholdCB );
    usethresholdfld_->attach( alignedBelow, stylefld_ );

    thresholdfld_ =  new uiGenInput( this, tr("Threshold size for Point mode"));
    thresholdfld_->attach( alignedBelow, usethresholdfld_ );
    mAttachCB( thresholdfld_->valuechanged, uiPickPropDlg::thresholdChangeCB );
    thresholdfld_->setSensitive( usethreshold );
    thresholdfld_->setValue( Pick::Set::getSizeThreshold() );

    uiColorInput::Setup colstu( set_.disp_.fillcolor_ );
    colstu.lbltxt( tr("Fill with") ).withcheck( true )
	  .transp(uiColorInput::Setup::Separate);
    fillcolfld_ = new uiColorInput( this, colstu );
    fillcolfld_->attach( alignedBelow, stylefld_ );
    fillcolfld_->attach( ensureBelow, thresholdfld_ );
    mAttachCB( fillcolfld_->colorChanged, uiPickPropDlg::fillColorChangeCB );
    mAttachCB( fillcolfld_->doDrawChanged, uiPickPropDlg::fillColorChangeCB );
}


uiPickPropDlg::~uiPickPropDlg()
{
    detachAllNotifiers();
}


void uiPickPropDlg::linePropertyChanged( CallBacker* )
{
    OD::LineStyle lst;
    lst = lsfld_->getStyle();

    set_.disp_.type_ = lst.type_;
    set_.disp_.width_ = lst.width_;
    set_.disp_.linecolor_ = lst.color_;
    set_.disp_.linedodraw_ = lsfld_->doDraw();
    if ( !lsfld_->doDraw() )
    {
	set_.disp_.connect_ = Pick::Set::Disp::None;
    }
    else
    {
	set_.disp_.connect_ = Pick::Set::Disp::Close;
	fillcolfld_->setDoDraw( false );
    }
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::fillColorChangeCB( CallBacker* )
{
    bool fillcol = fillcolfld_->doDraw();
    set_.disp_.fillcolor_ = fillcolfld_->color() ;
    set_.disp_.filldodraw_ = fillcolfld_->doDraw() ;
    if ( !fillcol )
    {
	if ( !psd_ )
	    return;
	psd_->displayBody( false );
    }
    else
    {
	if ( !psd_ )
	    return;

	set_.disp_.connect_ = Pick::Set::Disp::None ;
	lsfld_->setDoDraw( false );
	psd_->displayBody( true );
	if ( !psd_->getDisplayBody() )
	    psd_->setBodyDisplay();
    }
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
    MarkerStyle3D style;
    if ( !stylefld_->showMarker() )
	set_.disp_.markertype_ = MarkerStyle3D::None;
    else
    {
	stylefld_->getMarkerStyle( style );
	set_.disp_.markertype_ = style.type_;
    }
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
    set_.writeDisplayPars();
    return true;
}

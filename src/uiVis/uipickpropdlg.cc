/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"
#include "picksetmanager.h"
#include "taskrunner.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimarkerstyle.h"
#include "uiseparator.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"
#include "ioobj.h"
#include "dbman.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiDialog(p,Setup(tr("PointSet Display Properties"),
			mNoDlgTitle,mTODOHelpKey).savebutton(true)
			.savetext(tr("Save on OK")))
    , set_( set )
    , psd_( psd )
{
    set_.ref();
    setTitleText( uiString::empty() );
    setSaveButtonChecked( true );
    uiSelLineStyle::Setup stu;
    lsfld_ = new uiSelLineStyle( this, OD::LineStyle(set_.lineStyle()), stu );
    lsfld_->setDoDraw( set_.lineDoDraw() );
    mAttachCB( lsfld_->changed, uiPickPropDlg::linePropertyChanged );

    TypeSet<OD::MarkerStyle3D::Type> excl;
    excl.add( OD::MarkerStyle3D::None );
    stylefld_ = new uiMarkerStyle3D( this,
			uiMarkerStyle::Setup(),&excl );
    stylefld_->setMarkerStyle( set_.markerStyle() );
    mAttachCB( stylefld_->change, uiPickPropDlg::styleSel );
    stylefld_->attach( alignedBelow, lsfld_ );

    bool usethreshold = true;
    Settings::common().getYN( Pick::Set::sKeyUseThreshold(), usethreshold );
    usethresholdfld_ = new uiCheckBox( this,
			   tr("Switch to Point mode for all large PointSets") );
    usethresholdfld_->setChecked( usethreshold );
    mAttachCB( usethresholdfld_->activated, uiPickPropDlg::useThresholdCB );
    usethresholdfld_->attach( alignedBelow, stylefld_ );

    thresholdfld_ =  new uiGenInput( this, tr("Threshold size for Point mode"));
    thresholdfld_->attach( alignedBelow, usethresholdfld_ );
    mAttachCB( thresholdfld_->valuechanged, uiPickPropDlg::thresholdChangeCB );
    thresholdfld_->setSensitive( usethreshold );
    thresholdfld_->setValue( Pick::Set::getSizeThreshold() );
    uiColorInput::Setup colstu( set_.fillColor() );
    colstu.lbltxt( tr("Fill with") ).withcheck( true )
	  .transp(uiColorInput::Setup::Separate);
    fillcolfld_ = new uiColorInput( this, colstu );
    fillcolfld_->setDoDraw( set_.fillDoDraw() );
    fillcolfld_->attach( alignedBelow, stylefld_ );
    fillcolfld_->attach( ensureBelow, thresholdfld_ );

    mAttachCB( fillcolfld_->colorChanged, uiPickPropDlg::fillColorChangeCB );
    mAttachCB( fillcolfld_->doDrawChanged, uiPickPropDlg::fillColorChangeCB );
    mAttachCB( postFinalise(), uiPickPropDlg::initDlg );
}


uiPickPropDlg::~uiPickPropDlg()
{
    detachAllNotifiers();
    set_.unRef();
}


void uiPickPropDlg::initDlg( CallBacker* )
{
    NotifyStopper nsmarker( stylefld_->change  );
    NotifyStopper nsfillcols( fillcolfld_->colorChanged );
    NotifyStopper nsfilldodrawch( fillcolfld_->doDrawChanged );

    stylefld_->setMarkerStyle( set_.markerStyle() );
    fillcolfld_->setDoDraw( set_.fillDoDraw() );

    fillColorChangeCB(nullptr);
    linePropertyChanged(nullptr);
    styleSel(nullptr);
}

void uiPickPropDlg::linePropertyChanged( CallBacker* )
{
    const OD::LineStyle lst = lsfld_->getStyle();
    set_.setLineStyle( lst );
    set_.setLineDoDraw( lsfld_->doDraw() );
    if ( !lsfld_->doDraw() )
    {
	set_.setConnection(Pick::Set::Disp::None);
    }
    else
    {
	set_.setConnection(Pick::Set::Disp::Close);
	fillcolfld_->setDoDraw( false );
    }
}


void uiPickPropDlg::fillColorChangeCB( CallBacker* )
{
    bool fillcol = fillcolfld_->doDraw();
    set_.setFillColor( fillcolfld_->color() );
    set_.setFillDoDraw( fillcolfld_->doDraw() );
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

	set_.setConnection( Pick::Set::Disp::None );
	lsfld_->setDoDraw( false );
	psd_->displayBody( true );
	if ( !psd_->getDisplayBody() )
	    psd_->setBodyDisplay();
    }
}


void uiPickPropDlg::styleSel( CallBacker* )
{
    OD::MarkerStyle3D style( set_.markerStyle() );

    if ( !stylefld_->showMarker() )
	set_.setMarkerStyle( OD::MarkerStyle3D::None );
    else
    {
	stylefld_->getMarkerStyle( style );
	set_.setMarkerStyle( style );
    }
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
    if ( thresholdval == Pick::Set::getSizeThreshold() )
	return;

    Settings::common().set( Pick::Set::sKeyThresholdSize(), thresholdval );
    Settings::common().write();
}


bool uiPickPropDlg::acceptOK()
{
    if ( !saveButtonChecked() )	
	return true;

    IOPar par;
    set_.fillPar( par );
    DBKey dbkey = Pick::SetMGR().getID(set_).getIOObj()->key();
    if ( dbkey.isInvalid() )
	return false;

    return Pick::SetMGR().writeDisplayPars( dbkey, set_ );
}

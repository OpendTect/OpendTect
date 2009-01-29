/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerappearancetab.cc,v 1.3 2009-01-29 22:16:32 cvsyuancheng Exp $";

#include "uipsviewerappearancetab.h"

#include "coltabsequence.h"
#include "flatposdata.h"
#include "settings.h"
#include "uibutton.h"
#include "uicolortable.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipsviewermanager.h"
#include "visflatviewer.h"
#include "visprestackviewer.h"
#include "survinfo.h"
#include "samplingdata.h"


namespace PreStackView
{
uiViewer3DAppearanceTab::uiViewer3DAppearanceTab( uiParent* p,
			    PreStackView::Viewer3D& psv, uiViewer3DMgr& mgr )
    : uiDlgGroup( p, "Appearance" )
    , applyall_( false )
    , savedefault_( false )
    , vwr_( psv.flatViewer() )  
    , mgr_( mgr )					    
{
    uicoltab_ = new uiColorTable( this, 
	    vwr_ ? vwr_->appearance().ddpars_.vd_.ctab_.buf() : 0, false );
    uicoltablbl_ = new uiLabel( this, "Color table", uicoltab_ );

    const SamplingData<float> curzsmp = vwr_->appearance().annot_.x2_.sampling_;
    const bool curdefined = !mIsUdf( curzsmp.start ) && !mIsUdf(curzsmp.step);
    SamplingData<float> usedzsmp = curdefined ? curzsmp 
					      : vwr_->getGridSampling( false );
    usedzsmp.start *= SI().zFactor();
    usedzsmp.step *= SI().zFactor();

    zgridfld_ = new uiGenInput( this, "Z grid lines",
	    BoolInpSpec( vwr_->appearance().annot_.x2_.showgridlines_ ) );
    zgridfld_->attach( alignedBelow, uicoltab_ );
    zgridfld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateZFlds) );

    zgridautofld_ = new uiGenInput( this, 0, BoolInpSpec(
		mIsUdf(curzsmp.start) || mIsUdf(curzsmp.step),
		"Automatic sampling", 0, 0 ) );
    zgridautofld_->attach( rightOf, zgridfld_ );
    zgridautofld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateZFlds) );

    zgridrangefld_ = new uiGenInput( this, "Z grid sampling (start/step)",
	    FloatInpSpec(usedzsmp.start), FloatInpSpec(usedzsmp.step) );
    zgridrangefld_->attach( alignedBelow, zgridfld_ );

    zgridrangelbl_ = new uiLabel( this, SI().getZUnitString(true) );
    zgridrangelbl_->attach( rightOf, zgridrangefld_ );

    SamplingData<float> curoffssmp = vwr_->appearance().annot_.x1_.sampling_;
    SamplingData<float> usedoffssmp = curoffssmp;
    if ( mIsUdf( curoffssmp.start ) || mIsUdf(curoffssmp.step) )
	usedoffssmp = vwr_->getGridSampling( true );

    offsgridfld_ = new uiGenInput( this, "Offset grid lines",
	    BoolInpSpec( vwr_->appearance().annot_.x1_.showgridlines_ ) );
    offsgridfld_->attach( alignedBelow, zgridrangefld_ );
    offsgridfld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateOffsFlds) );

    offsgridautofld_ = new uiGenInput( this, 0, BoolInpSpec(
		mIsUdf(curoffssmp.start) || mIsUdf(curoffssmp.step),
		"Automatic sampling", 0, 0 ) );
    offsgridautofld_->attach( rightOf, offsgridfld_ );
    offsgridautofld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateOffsFlds) );

    offsgridrangefld_ = new uiGenInput( this, 
	    "Offset grid sampling (start/step)",
	    FloatInpSpec(usedoffssmp.start), FloatInpSpec(usedoffssmp.step) );
    offsgridrangefld_->attach( alignedBelow, offsgridfld_ );

    offsgridrangelbl_ = new uiLabel( this, SI().getXYUnitString(true) );
    offsgridrangelbl_->attach( rightOf, offsgridrangefld_ );

    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->activated.notify(
		mCB(this,uiViewer3DAppearanceTab,applyButPushedCB) );
    applybut_->attach( alignedBelow, offsgridrangefld_ );

    updateZFlds( 0 );
    updateOffsFlds( 0 );
}


void uiViewer3DAppearanceTab::updateOffsFlds( CallBacker* )
{
    const bool useoffsgrids = offsgridfld_->getBoolValue();
    offsgridautofld_->display( useoffsgrids );
    offsgridrangefld_->display( useoffsgrids );
    offsgridrangelbl_->display( useoffsgrids );

    if ( !useoffsgrids )
	return;

    const bool useauto = offsgridautofld_->getBoolValue();
    offsgridrangefld_->setSensitive( !useauto );
    SamplingData<float> sd = useauto ? vwr_->getGridSampling( true ) 
				     : vwr_->appearance().annot_.x1_.sampling_;
    if ( !useauto && (mIsUdf(sd.start) || mIsUdf(sd.step)) )
	sd = vwr_->getGridSampling( true );

    offsgridrangefld_->setValues( sd.start, sd.step );
}


void uiViewer3DAppearanceTab::updateZFlds( CallBacker* )
{
    const bool usezgrids = zgridfld_->getBoolValue();
    zgridautofld_->display( usezgrids );
    zgridrangefld_->display( usezgrids );
    zgridrangelbl_->display( usezgrids );

    if ( !usezgrids )
	return;

    const bool useauto = zgridautofld_->getBoolValue();
    zgridrangefld_->setSensitive( !useauto );
    SamplingData<float> sd = useauto ? vwr_->getGridSampling( false ) 
				     : vwr_->appearance().annot_.x2_.sampling_;	
    if ( !useauto && (mIsUdf(sd.start) || mIsUdf(sd.step)) )
	sd = vwr_->getGridSampling( false );
    
    sd.start *= SI().zFactor();
    sd.step *= SI().zFactor();
    zgridrangefld_->setValues( sd.start, sd.step );
}


bool uiViewer3DAppearanceTab::acceptOK()
{
    if ( !vwr_ )
	return true;

    IOPar flatviewpar;
    vwr_->appearance().ddpars_.fillPar( flatviewpar );
    if ( saveAsDefault() )
    {
	Settings& settings = Settings::fetch( uiViewer3DMgr::sSettings3DKey() );
	settings.mergeComp( flatviewpar, uiViewer3DMgr::sKeyFlatviewPars() );
	if ( !settings.write() )
	{
	    uiMSG().error("Cannot write  default settings");
	    return false;
	}
    }

    applyButPushedCB( 0 );
    return true;
}


void uiViewer3DAppearanceTab::applyButPushedCB( CallBacker* cb )
{
    if ( !vwr_ )
	return;

    vwr_->appearance().ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
    vwr_->handleChange( FlatView::Viewer::VDPars );

    const bool showzgridlines = zgridfld_->getBoolValue();
    vwr_->appearance().annot_.x2_.showgridlines_ = showzgridlines;
    SamplingData<float> zsmp(  mUdf(float),  mUdf(float) );
    if ( showzgridlines )
    {
	if ( !zgridautofld_->getBoolValue() )
	{
	    zsmp.start = zgridrangefld_->getfValue( 0 );
	    zsmp.step = zgridrangefld_->getfValue( 1 );
	    if ( mIsUdf(zsmp.start) || mIsUdf(zsmp.step) )
	    {
		uiMSG().error("Z sampling is not defined.");
		return;
	    }

	    const float zfac = SI().zFactor();
	    zsmp.start /= zfac;
	    zsmp.step /= zfac;
	}

	vwr_->appearance().annot_.x2_.sampling_ = zsmp;
    }

    const bool showoffsgridlines = offsgridfld_->getBoolValue();
    vwr_->appearance().annot_.x1_.showgridlines_ = showoffsgridlines;
    SamplingData<float> offssmp(  mUdf(float),  mUdf(float) );
    if ( showoffsgridlines )
    {
	if ( !offsgridautofld_->getBoolValue() )
	{
	    offssmp.start = offsgridrangefld_->getfValue( 0 );
	    offssmp.step = offsgridrangefld_->getfValue( 1 );
	    if ( mIsUdf(offssmp.start) || mIsUdf(offssmp.step) )
	    {
		uiMSG().error("Offset sampling is not defined.");
		return;
	    }
	}
	    
	vwr_->appearance().annot_.x1_.sampling_ = offssmp;
    }

    vwr_->handleChange( FlatView::Viewer::Annot );

    if ( !applyall_ )
	return;
    
    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	visBase::FlatViewer* fvwr = mgr_.get3DViewers()[idx]->flatViewer();
	if ( fvwr==vwr_ )
	    continue;

	fvwr->appearance().ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
	fvwr->handleChange( FlatView::Viewer::VDPars );
	
	fvwr->appearance().annot_.x2_.showgridlines_ = showzgridlines;
	fvwr->appearance().annot_.x1_.showgridlines_ = showoffsgridlines;
	if ( showzgridlines )
	    fvwr->appearance().annot_.x2_.sampling_ = zsmp;

	if ( showoffsgridlines )
	    fvwr->appearance().annot_.x1_.sampling_ = offssmp;

	fvwr->handleChange( FlatView::Viewer::Annot );
    }
}


}; //namespace


/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerappearancetab.cc,v 1.2 2009-01-26 16:08:15 cvsbert Exp $";

#include "uipsviewerappearancetab.h"

#include "coltabsequence.h"
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

    const float zfac = SI().zFactor();
    const bool xyinft = SI().xyInFeet();
    //TODO get from current settings
    SamplingData<float> curzsmp( SI().zRange(true).start * zfac, zfac );
    bool curhavezannot = false;
    SamplingData<float> curoffssmp( 0, xyinft ? 1000 : 2000 );
    bool curhaveoffsannot = false;

    zannotfld_ = new uiGenInput( this, "Z grid lines (start/step)",
		    FloatInpSpec(curzsmp.start), FloatInpSpec(curzsmp.step) );
    zannotfld_->attach( alignedBelow, uicoltab_ );
    zannotfld_->setWithCheck( true );
    zannotfld_->setChecked( curhavezannot );
    uiLabel* lbl = new uiLabel( this, SI().getZUnitString(true) );
    lbl->attach( rightOf, zannotfld_ );

    offsannotfld_ = new uiGenInput( this, "Offset grid lines (start/step)",
				    FloatInpSpec(curoffssmp.start),
				    FloatInpSpec(curoffssmp.step) );
    offsannotfld_->attach( alignedBelow, zannotfld_ );
    offsannotfld_->setWithCheck( true );
    offsannotfld_->setChecked( curhaveoffsannot );
    lbl = new uiLabel( this, xyinft ? "(ft)" : "(m)" );
    lbl->attach( rightOf, offsannotfld_ );

    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->activated.notify(
		mCB(this,uiViewer3DAppearanceTab,applyButPushedCB) );
    applybut_->attach( alignedBelow, offsannotfld_ );
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

    //TODO store in actual vis settings rather than local variables
    const float zfac = SI().zFactor();
    SamplingData<float> zsmp( 0, zfac );
    bool havezannot = zannotfld_->isChecked();
    if ( havezannot )
    {
	zsmp.start = zannotfld_->getfValue( 0 ) / zfac;
	zsmp.step = zannotfld_->getfValue( 1 ) / zfac;
    }
    SamplingData<float> offssmp( 0, 1000 );
    bool haveoffsannot = offsannotfld_->isChecked();
    if ( haveoffsannot )
    {
	offssmp.start = zannotfld_->getfValue( 0 );
	offssmp.step = zannotfld_->getfValue( 1 );
	/*
		TODO Something to think about:
		When coordinates are in feet, are offsets then also in feet?
		Ask Kris ...!
		If not, we need to scale here ...
	if ( SI().xyInFeet() )
	    { offssmp.start *= 0.3048; offssmp.step *= 0.3048; }
	*/
    }

    if ( !applyall_ )
	return;
    
    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	visBase::FlatViewer* fvwr = mgr_.get3DViewers()[idx]->flatViewer();
	if ( fvwr==vwr_ )
	    continue;

	fvwr->appearance().ddpars_.vd_.ctab_ = uicoltab_->colTabSeq().name();
	fvwr->handleChange( FlatView::Viewer::VDPars );
    }
}


}; //namespace


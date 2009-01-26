/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerappearancetab.cc,v 1.1 2009-01-26 15:09:08 cvsbert Exp $";

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
    //TODO get from current settings
    float curzstart = SI().zRange(true).start * zfac;
    float curzstep = 1 * zfac;
    bool curhavezannot = false;
    float curoffsstart = 0;
    float curoffsstep = 1000;
    bool curhaveoffsannot = false;

    zannotfld_ = new uiGenInput( this, "Z grid lines (start/step)",
	    		FloatInpSpec(curzstart), FloatInpSpec(curzstep) );
    zannotfld_->attach( alignedBelow, uicoltab_ );
    zannotfld_->setWithCheck( true );
    zannotfld_->setChecked( curhavezannot );

    offsannotfld_ = new uiGenInput( this, "Offset grid lines (start/step)",
	    		FloatInpSpec(curoffsstart), FloatInpSpec(curoffsstep) );
    offsannotfld_->attach( alignedBelow, zannotfld_ );
    offsannotfld_->setWithCheck( true );
    offsannotfld_->setChecked( curhaveoffsannot );

    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->activated.notify( mCB(this,uiViewer3DAppearanceTab,applyButPushedCB) );
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

    //TODO make grid lines choices effective

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


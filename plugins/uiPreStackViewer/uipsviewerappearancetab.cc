/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/

#include "uipsviewerappearancetab.h"

#include "coltabsequence.h"
#include "settings.h"
#include "uibutton.h"
#include "uicoltabsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipsviewermanager.h"
#include "visflatviewer.h"
#include "visprestackdisplay.h"
#include "survinfo.h"
#include "samplingdata.h"
#include "uistrings.h"

namespace PreStackView
{

uiViewer3DAppearanceTab::uiViewer3DAppearanceTab( uiParent* p,
		visSurvey::PreStackDisplay& psv, uiViewer3DMgr& mgr )
    : uiDlgGroup( p, tr("Appearance") )
    , applyall_( false )
    , savedefault_( false )
    , vwr_( psv.flatViewer() )
    , mgr_( mgr )
    , manuzsampl_( vwr_->appearance().annot_.x2_.sampling_ )
    , manuoffssampl_( vwr_->appearance().annot_.x1_.sampling_ )
{
    coltabsel_ = new uiColTabSel( this, OD::Horizontal,
				 uiStrings::sColorTable() );
    coltabsel_->setSeqName( vwr_->appearance().ddpars_.vd_.colseqname_ );
    coltabsel_->setMapper( *vwr_->appearance().ddpars_.vd_.mapper_ );
    mAttachCB( coltabsel_->seqChanged, uiViewer3DAppearanceTab::colTabChanged );

    const SamplingData<float> curzsmp = vwr_->appearance().annot_.x2_.sampling_;
    const bool zudf = mIsUdf( curzsmp.start ) || mIsUdf(curzsmp.step);

    zgridfld_ = new uiGenInput( this, tr("Z grid lines"),
	    BoolInpSpec( vwr_->appearance().annot_.x2_.showgridlines_ ) );
    zgridfld_->attach( alignedBelow, coltabsel_ );
    zgridfld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateZFlds) );

    zgridautofld_ = new uiGenInput( this, uiString::empty(),
	    BoolInpSpec( zudf, tr("Automatic sampling"),
			 uiString::empty(), 0 ) );
    zgridautofld_->attach( rightOf, zgridfld_ );
    zgridautofld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateZFlds) );

    zgridrangefld_ = new uiGenInput( this, tr("Z grid sampling (start/step)"),
	    FloatInpSpec(), FloatInpSpec() );
    zgridrangefld_->attach( alignedBelow, zgridfld_ );

    zgridrangelbl_ = new uiLabel( this, SI().zUnitString().parenthesize() );
    zgridrangelbl_->attach( rightOf, zgridrangefld_ );

    SamplingData<float> curoffssmp = vwr_->appearance().annot_.x1_.sampling_;
    const bool offsudf = mIsUdf(curoffssmp.start) || mIsUdf(curoffssmp.step);

    offsgridfld_ = new uiGenInput( this, tr("Offset grid lines"),
	    BoolInpSpec( vwr_->appearance().annot_.x1_.showgridlines_ ) );
    offsgridfld_->attach( alignedBelow, zgridrangefld_ );
    offsgridfld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateOffsFlds) );

    offsgridautofld_ = new uiGenInput( this, uiString::empty(),
	    BoolInpSpec( offsudf, tr("Automatic sampling"),
			 uiString::empty(), 0 ) );
    offsgridautofld_->attach( rightOf, offsgridfld_ );
    offsgridautofld_->valuechanged.notify(
	    mCB(this,uiViewer3DAppearanceTab,updateOffsFlds) );

    offsgridrangefld_ = new uiGenInput( this,
	    tr("Offset grid sampling (start/step)"),
            FloatInpSpec(),FloatInpSpec());
    offsgridrangefld_->attach( alignedBelow, offsgridfld_ );

    offsgridrangelbl_ = new uiLabel( this, SI().xyUnitString().parenthesize() );
    offsgridrangelbl_->attach( rightOf, offsgridrangefld_ );

    applybut_ = uiButton::getStd( this, OD::Apply,
		mCB(this,uiViewer3DAppearanceTab,applyButPushedCB), true );
    applybut_->attach( alignedBelow, offsgridrangefld_ );

    if ( mIsUdf(manuzsampl_.start) || mIsUdf(manuzsampl_.step) )
	manuzsampl_ = vwr_->getDefaultGridSampling( false );
    if ( mIsUdf(manuoffssampl_.start) || mIsUdf(manuoffssampl_.step) )
	manuoffssampl_ = vwr_->getDefaultGridSampling( true );

    mAttachCB( vwr_->dispParsChanged, uiViewer3DAppearanceTab::updateColTab );

    updateColTab( 0 );
    updateZFlds( 0 );
    updateOffsFlds( 0 );
}


uiViewer3DAppearanceTab::~uiViewer3DAppearanceTab()
{
    detachAllNotifiers();
}


void uiViewer3DAppearanceTab::colTabChanged( CallBacker* )
{
    FlatView::DataDispPars::VD& pars = vwr_->appearance().ddpars_.vd_;
    pars.colseqname_ = coltabsel_->seqName();
    *pars.mapper_ = coltabsel_->mapper();
    vwr_->handleChange( FlatView::Viewer::DisplayPars );
}


void uiViewer3DAppearanceTab::updateColTab( CallBacker* )
{
    FlatView::DataDispPars::VD& pars = vwr_->appearance().ddpars_.vd_;
    coltabsel_->setMapper( *pars.mapper_ );
    coltabsel_->setSeqName( pars.colseqname_ );
    coltabsel_->setRange( vwr_->getDataRange(false) );
}


void uiViewer3DAppearanceTab::updateOffsFlds( CallBacker* )
{
    updateFlds( offsgridfld_, offsgridautofld_, offsgridrangefld_,
		offsgridrangelbl_, true );
}


void uiViewer3DAppearanceTab::updateZFlds( CallBacker* )
{
    updateFlds( zgridfld_,zgridautofld_,zgridrangefld_,zgridrangelbl_,false );
}


void uiViewer3DAppearanceTab::updateFlds( uiGenInput* gridfld,
					  uiGenInput* autofld,
					  uiGenInput* rgfld,
					  uiLabel* lblfld, bool x1 )
{
    const bool usegrids = gridfld->getBoolValue();
    autofld->display( usegrids );
    rgfld->display( usegrids );
    lblfld->display( usegrids );

    if ( !usegrids )
	return;

    const bool useauto = autofld->getBoolValue();
    rgfld->setSensitive( !useauto );
    SamplingData<float> sd = useauto ? vwr_->getDefaultGridSampling( x1 )
				     : (x1 ? manuoffssampl_ : manuzsampl_);
    if ( !x1 )
    {
	sd.start *= SI().zDomain().userFactor();
	sd.step *= SI().zDomain().userFactor();
    }

    rgfld->setValues( sd.start, sd.step );
}


bool uiViewer3DAppearanceTab::acceptOK()
{
    if ( !vwr_ )
	return true;

    applyButPushedCB( 0 );

    IOPar flatviewpar;
    vwr_->appearance().ddpars_.fillPar( flatviewpar );
    vwr_->appearance().annot_.fillPar( flatviewpar );
    if ( saveAsDefault() )
    {
	Settings& settings = Settings::fetch( uiViewer3DMgr::sSettings3DKey() );
	settings.mergeComp( flatviewpar, uiViewer3DMgr::sKeyFlatviewPars() );
	if ( !settings.write() )
	{
	    uiMSG().error(tr("Cannot write  default settings"));
	    return false;
	}
    }

    return true;
}


void uiViewer3DAppearanceTab::applyButPushedCB( CallBacker* cb )
{
    if ( !vwr_ )
	return;

    FlatView::DataDispPars& ddp = vwr_->appearance().ddpars_;
    *ddp.vd_.mapper_ = coltabsel_->mapper();
    ddp.vd_.colseqname_ = coltabsel_->seqName();
    vwr_->handleChange( FlatView::Viewer::DisplayPars );

    const bool showzgridlines = zgridfld_->getBoolValue();
    vwr_->appearance().annot_.x2_.showgridlines_ = showzgridlines;

    SamplingData<float> zsmp( mUdf(float), mUdf(float) );
    if ( showzgridlines )
    {
	if ( !zgridautofld_->getBoolValue() )
	{
	    zsmp.start = zgridrangefld_->getFValue( 0 );
	    zsmp.step = zgridrangefld_->getFValue( 1 );
	    if ( mIsUdf(zsmp.start) || mIsUdf(zsmp.step) )
	    {
		if ( mIsUdf(zsmp.start) )
		    uiMSG().error(tr("Z sampling start is not defined."));
		else
		    uiMSG().error(tr("Z sampling step is not defined."));
		return;
	    }

	    const float zfac = mCast( float, SI().zDomain().userFactor() );
	    zsmp.start /= zfac;
	    zsmp.step /= zfac;
	    manuzsampl_ = zsmp;
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
	    offssmp.start = offsgridrangefld_->getFValue( 0 );
	    offssmp.step = offsgridrangefld_->getFValue( 1 );
	    if ( mIsUdf(offssmp.start) || mIsUdf(offssmp.step) )
	    {
		if ( mIsUdf(offssmp.start) )
		    uiMSG().error(tr("Offset sampling start is not defined."));
		else
		    uiMSG().error(tr("Offset sampling step is not defined."));
		return;
	    }

	    manuoffssampl_ = offssmp;
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

	fvwr->appearance().ddpars_.vd_.colseqname_ = coltabsel_->seqName();

	fvwr->appearance().annot_.x2_.showgridlines_ = showzgridlines;
	fvwr->appearance().annot_.x1_.showgridlines_ = showoffsgridlines;
	if ( showzgridlines )
	    fvwr->appearance().annot_.x2_.sampling_ = zsmp;

	if ( showoffsgridlines )
	    fvwr->appearance().annot_.x1_.sampling_ = offssmp;

	fvwr->handleChange( FlatView::Viewer::Annot |
			    FlatView::Viewer::DisplayPars );
    }
}

} // namespace PreStackView

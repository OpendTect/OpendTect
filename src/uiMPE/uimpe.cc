/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimpe.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispreload.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseispreloadmgr.h"
#include "uitaskrunner.h"

namespace MPE
{

// uiTrackSettingsValidator
uiTrackSettingsValidator::uiTrackSettingsValidator()
{}


uiTrackSettingsValidator::~uiTrackSettingsValidator()
{}


bool uiTrackSettingsValidator::checkInVolumeTrackMode() const
{
    return true;
}


bool uiTrackSettingsValidator::checkActiveTracker() const
{
    if ( !MPE::engine().getActiveTracker() )
    {
	uiMSG().error( tr("Please select a horizon in the tree") );
	return false;
    }

    return true;
}


bool uiTrackSettingsValidator::checkStoredData( Attrib::SelSpec& as,
						MultiID& key ) const
{
    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet( false, true );
    ConstRefMan<Attrib::Desc> desc = ads ? ads->getDesc( as.id() ) : nullptr;
    if ( !desc )
    {
	ads = Attrib::DSHolder().getDescSet( false, false );
	desc = ads ? ads->getDesc( as.id() ).ptr() : nullptr;
    }

    key = desc ? desc->getStoredID(false) : MultiID::udf();
    if ( key.isUdf() )
    {
	uiMSG().error(
		tr("Volume tracking can only be done on stored volumes.") );
	return false;
    }

    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot find picked data in database") );
	return false;
    }

    return true;
}


bool uiTrackSettingsValidator::checkPreloadedData( const MultiID& key ) const
{
    if ( Seis::PLDM().isPresent(key) )
	return true;

    SeisIOObjInfo info( key );
    DataCharacteristics dc; info.getDataChar( dc );

    uiMain& uimain = uiMain::instance();
    uiDialog dlg( uimain.topLevel(),
	uiDialog::Setup(tr("Pre-load Data"),mNoDlgTitle,mTODOHelpKey) );
    auto* lbl1 = new uiLabel( &dlg,
	tr("Seeds have been picked on '%1'.").arg(IOM().nameOf(key)) );
    auto* lbl2 = new uiLabel( &dlg,
	tr("For Auto Tracking, this volume has to be pre-loaded") );
    lbl2->attach( alignedBelow, lbl1 );
    auto* grp = new uiButtonGroup( &dlg, "Pre-load options", OD::Vertical );
    grp->attach( alignedBelow, lbl2 );
    const StringView usertypestr =
		DataCharacteristics::toString( dc.userType() );
    uiString txt1 = tr("Pre-load Full Volume");
    if ( usertypestr.size() > 4 )
	txt1.append( tr("as %1").arg(usertypestr.buf()+4) );
    new uiCheckBox( grp, txt1 );
    new uiCheckBox( grp, tr("Choose Pre-load Parameters") );
    grp->selectButton(0);
    dlg.showAlwaysOnTop();
    if ( !dlg.go() )
	return false;

    uiTaskRunner uitr( uimain.topLevel() );
    int selbutid = grp->selectedId();
    if ( selbutid==0 )
    {
	TrcKeyZSampling tkzs; tkzs.setEmpty();
	Seis::PreLoader spl( key );
	spl.setTaskRunner( uitr );
	if ( !spl.load(tkzs) )
	{
	    const bool res = uiMSG().askGoOn(
		tr("Data has not been pre-loaded.\n"
		   "Do you want to pre-load with more options?") );
	    if ( res )
		selbutid = 1;
	    else
		return false;
	}
    }

    if ( selbutid==1 )
    {
	uiSeisPreLoadSel spldlg( uimain.topLevel(), Seis::Vol, key );
	spldlg.showAlwaysOnTop();
	if ( !spldlg.go() )
	    return false;

	TrcKeyZSampling tkzs; spldlg.getSampling( tkzs );
	DataCharacteristics spldc; spldlg.getDataChar( spldc );
	Seis::PreLoader spl( key );
	spl.setTaskRunner( uitr );
	if ( !spl.load(tkzs,spldc.userType(),spldlg.getScaler()) )
	{
	    const uiString emsg = spl.errMsg();
	    if ( !emsg.isEmpty() )
		uiMSG().error( emsg );
	    return false;
	}
    }

    return Seis::PLDM().isPresent( key );
}


// uiSetupGroup

uiSetupGroup::uiSetupGroup( uiParent* p, const char* ref )
    : uiGroup( p, "Tracking Setup" )
    , helpref_(ref)
{}


uiSetupGroup::~uiSetupGroup()
{}


bool uiSetupGroup::commitToTracker() const
{
    bool dummybool;
    return commitToTracker( dummybool );
}


// uiMPEEngine

uiMPEEngine::uiMPEEngine()
{}


uiMPEEngine::~uiMPEEngine()
{}


uiMPEEngine& uiMPE()
{
    mDefineStaticLocalObject( uiMPEEngine, uiengine,  );
    return uiengine;
}

} // namespace MPE

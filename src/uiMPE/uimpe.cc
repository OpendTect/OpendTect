/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/

#include "uimpe.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "cubesubsel.h"
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
bool uiTrackSettingsValidator::checkInVolumeTrackMode() const
{
    return true;
}


bool uiTrackSettingsValidator::checkActiveTracker() const
{
    if ( !MPE::engine().getActiveTracker() )
    {
	gUiMsg().error( tr("Please select a horizon in the tree") );
	return false;
    }

    return true;
}


bool uiTrackSettingsValidator::checkStoredData( Attrib::SelSpec& as,
						DBKey& key ) const
{
    const Attrib::DescSet& ads = Attrib::DescSet::global( false );
    const Attrib::Desc* desc = ads.getDesc( as.id() );

    key = desc ? DBKey(desc->getStoredID(false)) : DBKey::getInvalid();
    if ( key.isInvalid() )
    {
	gUiMsg().error(
		tr("Volume tracking can only be done on stored volumes.") );
	return false;
    }

    PtrMan<IOObj> ioobj = key.getIOObj();
    if ( !ioobj )
    {
	gUiMsg().error( tr("Cannot find picked data in database") );
	return false;
    }

    return true;
}


bool uiTrackSettingsValidator::checkPreloadedData( const DBKey& key ) const
{
    if ( Seis::PLDM().isPresent(key) )
	return true;

    SeisIOObjInfo info( key );
    DataCharacteristics dc; info.getDataChar( dc );

    uiMain& uimain = uiMain::theMain();
    uiDialog dlg( uimain.topLevel(),
	uiDialog::Setup(tr("Pre-load Data"),mNoDlgTitle,mTODOHelpKey) );
    uiLabel* lbl1 = new uiLabel( &dlg,
	tr("Seeds have been picked on '%1'.").arg(key.name()) );
    uiLabel* lbl2 = new uiLabel( &dlg,
	tr("For Auto Tracking, this volume has to be pre-loaded") );
    lbl2->attach( alignedBelow, lbl1 );
    uiButtonGroup* grp =
	new uiButtonGroup( &dlg, "Pre-load options", OD::Vertical );
    grp->attach( alignedBelow, lbl2 );
    const FixedString usertypestr =
		DataCharacteristics::toString( dc.userType() );
    uiString txt1;
    if ( usertypestr.size() > 4 )
	txt1 = tr("Pre-load Full Volume as %1").arg(usertypestr.buf()+4);
    else
	txt1 = tr("Pre-load Full Volume");

    new uiCheckBox( grp, txt1 );
    new uiCheckBox( grp, tr("Choose Pre-load Parameters") );
    grp->selectButton(0);
    dlg.showAlwaysOnTop();
    if ( !dlg.go() ) return false;

    uiTaskRunnerProvider uitr( uimain.topLevel() );
    int selbutid = grp->selectedId();
    if ( selbutid==0 )
    {
	TrcKeyZSampling tkzs; tkzs.setEmpty();
	const CubeSubSel css( tkzs );
	Seis::PreLoader spl( key );
	spl.setTaskRunner( uitr );
	if ( !spl.load(&css) )
	{
	    const bool res = gUiMsg().askGoOn(
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
	const CubeSubSel css( tkzs );
	DataCharacteristics spldc; spldlg.getDataChar( spldc );
	Seis::PreLoader spl( key );
	spl.setTaskRunner( uitr );
	if ( !spl.load(&css,spldc.userType(),spldlg.getScaler()) )
	{
	    const uiString emsg = spl.errMsg();
	    if ( !emsg.isEmpty() )
		gUiMsg().error( emsg );
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


bool uiSetupGroup::commitToTracker() const
{
    bool dummybool;
    return commitToTracker( dummybool );
}


void uiSetupGroupFactory::addFactory( uiSetupGrpCreationFunc f,
				      const char* name )
{
   names_.add( name );
   funcs += f;
}


uiSetupGroup* uiSetupGroupFactory::create( const char* name, uiParent* p,
					   const char* typestr )
{
    const int idx = names_.indexOf( name );
    if ( idx == -1 ) return 0;

    uiSetupGroup* res = funcs[idx](p,typestr);
    if ( res ) return res;

    return 0;
}


uiMPEEngine& uiMPE()
{
    mDefineStaticLocalObject( uiMPEEngine, uiengine,  );
    return uiengine;
}

} // namespace MPE

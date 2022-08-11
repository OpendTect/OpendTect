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
    const Attrib::Desc* desc = ads ? ads->getDesc( as.id() ) : 0;
    if ( !desc )
    {
	ads = Attrib::DSHolder().getDescSet( false, false );
	desc = ads ? ads->getDesc( as.id() ) : 0;
    }

    key = desc ? MultiID(desc->getStoredID(false)) : MultiID::udf();
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
    uiLabel* lbl1 = new uiLabel( &dlg,
	tr("Seeds have been picked on '%1'.").arg(IOM().nameOf(key)) );
    uiLabel* lbl2 = new uiLabel( &dlg,
	tr("For Auto Tracking, this volume has to be pre-loaded") );
    lbl2->attach( alignedBelow, lbl1 );
    uiButtonGroup* grp =
	new uiButtonGroup( &dlg, "Pre-load options", OD::Vertical );
    grp->attach( alignedBelow, lbl2 );
    const StringView usertypestr =
		DataCharacteristics::toString( dc.userType() );
    uiString txt1 = tr("Pre-load Full Volume");
    if ( usertypestr.size() > 4 )
	txt1.append( tr(" as %1").arg(usertypestr.buf()+4) );
    new uiCheckBox( grp, txt1 );
    new uiCheckBox( grp, tr("Choose Pre-load Parameters") );
    grp->selectButton(0);
    dlg.showAlwaysOnTop();
    if ( !dlg.go() ) return false;

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


void uiSetupGroupFactory::remove( const char* nm )
{
    const int idx = names_.indexOf( nm );
    if ( idx == -1 )
	return;

    names_.removeSingle( idx );
    funcs.removeSingle( idx );
}


uiMPEEngine& uiMPE()
{
    mDefineStaticLocalObject( uiMPEEngine, uiengine,  );
    return uiengine;
}

} // namespace MPE

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. Sen
 Date:          Jan 2017
________________________________________________________________________

-*/


#include "uinewemobjdlg.h"

#include "color.h"
#include "emmanager.h"
#include "od_helpids.h"
#include "randcolor.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistrings.h"


mImplClassFactory( uiNewEMObjectDlg, factory )

uiNewEMObjectDlg::uiNewEMObjectDlg( uiParent* p, const uiString& typ )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCreateNew(typ)
				,mNoDlgTitle,mTODOHelpKey))
    , emobj_(0)

{
    nmfld_ = new uiGenInput( this, tr("Name for new %1").arg(typ) );
    colorselfld_ = new uiColorInput( this,
	    uiColorInput::Setup(getRandomColor()).lbltxt(uiStrings::sColor()) );
    colorselfld_->attach( alignedBelow, nmfld_ );
}


uiPhrase uiNewEMObjectDlg::phrAlreadyLoadedAskForRename()
{
    return tr("Interpretation data is already loaded. Enter a different name" );
}


uiPhrase uiNewEMObjectDlg::phrInterpretationDataExist( const uiWord& typ,
						       const char* nm )
{
    return tr("A %1 with name '%2' already exists").arg( typ ).arg( nm );
}



uiNewFSSDlg::uiNewFSSDlg( uiParent* p )
    : uiNewEMObjectDlg(p,toUiString("FaultStickSet"))
{
}

RefMan<EM::Object> uiNewFSSDlg::getNewEMObject() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = EM::FSSMan().nameExists( nm );
    RefMan<EM::FaultStickSet> fssret;
    if ( !isreplace )
	fssret = new EM::FaultStickSet( nm );
    else
    {
	const DBKey emid = EM::FSSMan().getIDByName( nm );
	uiString msg = phrInterpretationDataExist( uiStrings::sFaultStickSet(),
						    nm);
	if ( EM::FSSMan().isLoaded(emid) )
	    { msg.appendPhrase(phrAlreadyLoadedAskForRename()); return fssret; }
	msg.appendPhrase( uiStrings::phrOutputFileExistsOverwrite() );
	if ( !uiMSG().askGoOn( msg ) )
	    return fssret;

	SilentTaskRunnerProvider trprov;
	fssret = EM::FSSMan().fetchForEdit( emid, trprov );
	if ( !fssret )
	    fssret = new EM::FaultStickSet( nm );
	else
	    fssret->removeAll();
    }

    return fssret;
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiNewFSSDlg::acceptOK()
{
    RefMan<EM::Object> newfss = getNewEMObject();
    emobj_= newfss;
    if ( !emobj_ )
	return false;

    emobj_->setPreferredColor( colorselfld_->color() );
    SilentTaskRunnerProvider trprov;
    uiString errmsg = EM::FSSMan().store( *emobj_, trprov );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}


RefMan<EM::FaultStickSet> uiNewFSSDlg::getFSS()
{
    mDynamicCastGet(EM::FaultStickSet*,fss,emobj_.ptr())
    return fss;
}


//Fault3D
uiNewFlt3DDlg::uiNewFlt3DDlg( uiParent* p )
    : uiNewEMObjectDlg(p,toUiString("Fault3D"))
{
}


RefMan<EM::Object> uiNewFlt3DDlg::getNewEMObject() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = EM::Flt3DMan().nameExists( nm );
    RefMan<EM::Fault3D> flt3d;
    if ( !isreplace )
	flt3d = new EM::Fault3D( nm );
    else
    {
	const DBKey emid = EM::Flt3DMan().getIDByName( nm );
	uiString msg = phrInterpretationDataExist( uiStrings::sFault(), nm);
	if ( EM::Flt3DMan().isLoaded(emid) )
	    { msg.appendPhrase(phrAlreadyLoadedAskForRename()); return flt3d; }
	msg.appendPhrase( uiStrings::phrOutputFileExistsOverwrite() );
	if ( !uiMSG().askGoOn( msg ) )
	    return flt3d;

	SilentTaskRunnerProvider trprov;
	flt3d = EM::Flt3DMan().fetchForEdit( emid, trprov );
	if ( !flt3d )
	    flt3d = new EM::FaultStickSet( nm );
	else
	    flt3d->removeAll();
    }

    return flt3d;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiNewFlt3DDlg::acceptOK()
{
    RefMan<EM::Object> newflt3d = getNewEMObject();
    emobj_= newflt3d;
    SilentTaskRunnerProvider trprov;
    uiString errmsg = EM::Flt3DMan().store( *emobj_, trprov );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}


RefMan<EM::Fault3D> uiNewFlt3DDlg::getFault3D()
{
    mDynamicCastGet(EM::Fault3D*,flt,emobj_.ptr())
    return flt;
}


//Horizon3D

uiNewHorizon3DDlg::uiNewHorizon3DDlg( uiParent* p )
    : uiNewEMObjectDlg(p,uiStrings::sHorizon())
{
}

RefMan<EM::Object> uiNewHorizon3DDlg::getNewEMObject() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = EM::Hor3DMan().nameExists( nm );
    RefMan<EM::Horizon3D> horret;
    if ( !isreplace )
	horret = new EM::Horizon3D( nm );
    else
    {
	const DBKey emid = EM::Hor3DMan().getIDByName( nm );
	uiString msg = phrInterpretationDataExist( uiStrings::sHorizon(), nm );
	if ( EM::Hor3DMan().isLoaded(emid) )
	    { msg.appendPhrase(phrAlreadyLoadedAskForRename()); return horret; }
	msg.appendPhrase( uiStrings::phrOutputFileExistsOverwrite() );
	if ( !uiMSG().askGoOn( msg ) )
	    return horret;

	SilentTaskRunnerProvider trprov;
	horret = EM::Hor3DMan().fetchForEdit( emid, trprov );
	if ( !horret )
	    horret = new EM::Horizon3D( nm );
	else
	    horret->removeAll();
    }

    return horret;
}


bool uiNewHorizon3DDlg::acceptOK()
{
    RefMan<EM::Object> newhor = getNewEMObject();
    emobj_= newhor;
    if ( !emobj_ )
	return false;

    emobj_->setPreferredColor( colorselfld_->color() );
    SilentTaskRunnerProvider trprov;
    uiString errmsg = EM::Hor3DMan().store( *emobj_, trprov );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}


RefMan<EM::Horizon3D> uiNewHorizon3DDlg::getHorizon3D()
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj_.ptr())
    return hor3d;
}


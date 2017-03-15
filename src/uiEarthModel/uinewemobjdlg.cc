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

mImplFactory1Param( uiNewEMObjectDlg, uiParent*, uiNewEMObjectDlg::factory )

uiNewEMObjectDlg::uiNewEMObjectDlg( uiParent* p, const uiString& typ )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCreateNew(typ)
				,mNoDlgTitle,mTODOHelpKey))
    , emobj_(0)
    
{
    nmfld_ = new uiGenInput( this,
		tr("Name for new %1").arg(typ) );
    colorselfld_ = new uiColorInput( this,
				    uiColorInput::Setup(getRandomColor())
				    .lbltxt(uiStrings::sColor()) );
    colorselfld_->attach( alignedBelow, nmfld_ );
}



uiNewFSSDlg::uiNewFSSDlg( uiParent* p )
    : uiNewEMObjectDlg(p,toUiString("FaultStickSet"))
{
}

RefMan<EM::EMObject> uiNewFSSDlg::getNewEMObject() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = EM::FSSMan().nameExists( nm );
    RefMan<EM::FaultStickSet> fssret;
    if ( !isreplace )
	fssret = new EM::FaultStickSet( nm );
    else
    {
	const DBKey emid = EM::FSSMan().getIDByName( nm );
	uiString msg = tr("A FaultStickSet with that name already exists.\n");
	if ( EM::FSSMan().isLoaded(emid) )
	{
	    msg.append( tr("You are currently using it."
			"\nPlease enter a different name."), true );
	    return fssret;
	}
	msg.append( tr("Do you want to overwrite the existing data?"), true );
	if ( !uiMSG().askGoOn( msg ) )
	    return fssret;

	fssret = EM::FSSMan().fetchForEdit( emid );
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
    RefMan<EM::EMObject> newfss = getNewEMObject();
    emobj_= newfss;
    if ( !emobj_ )
	return false;

    emobj_->setPreferredColor( colorselfld_->color() );
    uiString errmsg = EM::FSSMan().store( *emobj_ );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}


//Fault3D
uiNewFlt3DDlg::uiNewFlt3DDlg( uiParent* p )
    : uiNewEMObjectDlg(p,toUiString("Fault3D"))
{
}


RefMan<EM::EMObject> uiNewFlt3DDlg::getNewEMObject() const
{
    const BufferString nm( nmfld_->text() );
    bool isreplace = EM::Flt3DMan().nameExists( nm );
    RefMan<EM::Fault3D> flt3d;
    if ( !isreplace )
	flt3d = new EM::Fault3D( nm );
    else
    {
	const DBKey emid = EM::Flt3DMan().getIDByName( nm );
	uiString msg = tr("A Fault3d with that name already exists.\n");
	if ( EM::Flt3DMan().isLoaded(emid) )
	{
	    msg.append( tr("You are currently using it."
			"\nPlease enter a different name."), true );
	    return flt3d;
	}
	msg.append( tr("Do you want to overwrite the existing data?"), true );
	if ( !uiMSG().askGoOn( msg ) )
	    return flt3d;

	flt3d = EM::Flt3DMan().fetchForEdit( emid );
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
    RefMan<EM::EMObject> newflt3d = getNewEMObject();
    emobj_= newflt3d;
    if ( !emobj_ )
	return false;

    emobj_->setPreferredColor( colorselfld_->color() );
    uiString errmsg = EM::Flt3DMan().store( *emobj_ );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg )

    return true;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. Sen
 Date:          Jan 2017
________________________________________________________________________

-*/


#include "uinewemobjdlg.h"

#include "color.h"
#include "emfaultstickset.h"
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

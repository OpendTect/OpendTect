/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.cc,v 1.4 2007-01-30 09:06:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uihor3dfrom2ddlg.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uiexecutor.h"
#include "uimsg.h"

#include "emhorizon2d.h"
#include "emhorizon.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhor2dto3d.h"
#include "array2dinterpol.h"
#include "ctxtioobj.h"
#include "survinfo.h"

static int nrsteps = 10;

uiHor3DFrom2DDlg::uiHor3DFrom2DDlg( uiParent* p, const EM::Horizon2D& h2d )
    : uiDialog( p, Setup("Create 3D Horizon","Specify parameters","0.0.0") )
    , hor2d_( h2d )
    , ctio_(*mMkCtxtIOObj(EMHorizon))
{
    ctio_.ctxt.forread = false;

    nriterfld_ = new uiGenInput( this, "Maximum interpolation steps",
	    			IntInpSpec(nrsteps) );
    outfld_ = new uiIOObjSel( this, ctio_, "Output Horizon" );
    outfld_->attach( alignedBelow, nriterfld_ );
    displayfld_ = new uiCheckBox( this, "Display after import" );
    displayfld_->attach( alignedBelow, outfld_ );
}


uiHor3DFrom2DDlg::~uiHor3DFrom2DDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiHor3DFrom2DDlg::acceptOK( CallBacker* )
{
#define mErrRet(s) { uiMSG().error(s); return false; }
    if ( !uiMSG().askGoOn( "Lots of TODOs ... are you sure?") )
	return false;

    if ( !outfld_->commitInput( true ) )
	mErrRet( "Please enter a name for the output horizon" )

    const char* nm = outfld_->getInput();
    EM::EMManager& em = EM::EMM();
    EM::ObjectID emobjid = em.createObject( EM::Horizon::typeStr(), nm );
    mDynamicCastGet(EM::Horizon*,hor3d,em.getObject(emobjid));
    if ( !hor3d )
	mErrRet( "Cannot create 3D horizon" );
    hor3d->setPreferredColor( hor2d_.preferredColor() );

    Executor* exec = new EM::Hor2DTo3D( hor2d_, SI().sampling(true).hrg,
	    			        nriterfld_->getIntValue(), *hor3d );
    uiExecutor* interpdlg = new uiExecutor( this, *exec );
    bool rv = interpdlg->go();
    delete exec; exec = 0;
    delete interpdlg;
#undef mErrRet
#define mErrRet() { hor3d->unRef(); delete exec; return false; }
    if ( !rv ) mErrRet()

    exec = hor3d->saver();
    if ( !exec ) mErrRet()

    uiExecutor dlg( this, *exec );
    rv = dlg.execute();
    delete exec;

    if ( !rv || !displayfld_->isChecked() )
	hor3d->unRef();
    else
	hor3d->unRefNoDelete();

    return rv;
}

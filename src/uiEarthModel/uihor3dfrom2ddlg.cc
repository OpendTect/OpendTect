/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.cc,v 1.14 2008-02-06 10:20:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "uihor3dfrom2ddlg.h"

#include "uiempartserv.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhor2dto3d.h"
#include "array2dinterpol.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "survinfo.h"

static int nrsteps = 10;
static float srchrad = -1;

uiHor3DFrom2DDlg::uiHor3DFrom2DDlg( uiParent* p, const EM::Horizon2D& h2d,
				    uiEMPartServer* ems )
    : uiDialog( p, Setup("Create 3D Horizon","Specify parameters","104.0.5") )
    , hor2d_( h2d )
    , emserv_( ems )
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , selid_( -1 )
{
    ctio_.ctxt.forread = false;
    if ( srchrad < 0 )
	srchrad = SI().inlDistance() * 40;

    grdtypfld_ = new uiGenInput( this, "Gridding type",
	    		BoolInpSpec(true,"Inverse distance","Extension") );
    grdtypfld_->valuechanged.notify( mCB(this,uiHor3DFrom2DDlg,typChg) );
    nriterfld_ = new uiGenInput( this, "Maximum extension steps",
	    			IntInpSpec(nrsteps) );
    nriterfld_->attach( alignedBelow, grdtypfld_ );
    srchradfld_ = new uiGenInput( this, "Search radius", FloatInpSpec(srchrad));
    srchradfld_->attach( alignedBelow, grdtypfld_ );

    outfld_ = new uiIOObjSel( this, ctio_, "Output Horizon" );
    outfld_->attach( alignedBelow, nriterfld_ );
    displayfld_ = new uiCheckBox( this, "Display after generation" );
    displayfld_->attach( alignedBelow, outfld_ );
}


uiHor3DFrom2DDlg::~uiHor3DFrom2DDlg()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiHor3DFrom2DDlg::typChg( CallBacker* )
{
    const bool isgrd = grdtypfld_->getBoolValue();
    nriterfld_->display( !isgrd );
    srchradfld_->display( isgrd );
}


MultiID uiHor3DFrom2DDlg::getSelID() const
{ return selid_; }


bool uiHor3DFrom2DDlg::doDisplay() const
{ return displayfld_->isChecked(); } 


#define mAskGoOnStr(nameandtypeexist) \
    ( nameandtypeexist ? \
	"An object with this name exists. Overwrite?" : \
	"An object of different type owns the name chosen.\n" \
	"Extend the chosen name to make it unique?" )

bool uiHor3DFrom2DDlg::acceptOK( CallBacker* )
{
#define mErrRet(s) { uiMSG().error(s); return false; }

    outfld_->processInput();
    const char* nm = outfld_->getInput();
    const BufferString typ = EM::Horizon3D::typeStr();

    PtrMan<IOObj> ioobj = IOM().getLocal( nm );
    const bool implexists = ioobj && ioobj->implExists( false );
    const bool nameandtypeexist = ioobj && typ==ioobj->group();
    if ( implexists && !uiMSG().askGoOn(mAskGoOnStr(nameandtypeexist),true) )
	return false;
    
    EM::EMManager& em = EM::EMM();

    if ( nameandtypeexist )
	emserv_->removeTreeObject( em.getObjectID(ioobj->key()) );
    
    const EM::ObjectID emobjid = em.createObject( typ, nm );
    mDynamicCastGet(EM::Horizon3D*,hor3d,em.getObject(emobjid));
    if ( !hor3d )
	mErrRet( "Cannot create 3D horizon" );

    hor3d->ref();
    hor3d->setPreferredColor( hor2d_.preferredColor() );

    EM::Hor2DTo3D::Setup setup( grdtypfld_->getBoolValue() );
    setup.srchrad( srchradfld_->getfValue() )
	 .nrsteps( nriterfld_->getIntValue() );
    Executor* exec = new EM::Hor2DTo3D( hor2d_, setup, *hor3d );
    uiTaskRunner* taskrunner = new uiTaskRunner( this );
    bool rv = taskrunner->execute( *exec );

    delete exec; exec = 0;
    delete taskrunner;
#undef mErrRet
#define mErrRet() { hor3d->unRef(); delete exec; return false; }
    if ( !rv ) mErrRet()

    exec = hor3d->saver();
    if ( !exec ) mErrRet()

    uiTaskRunner savedlg( this );
    rv = savedlg.execute( *exec );
    delete exec;

    if ( !rv || !doDisplay() )
	hor3d->unRef();
    else
    {
	selid_ = hor3d->multiID();
	hor3d->unRefNoDelete();
    }

    return rv;
}

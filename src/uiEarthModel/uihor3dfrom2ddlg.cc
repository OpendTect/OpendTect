/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          January 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihor3dfrom2ddlg.cc,v 1.21 2009-11-13 17:33:18 cvsyuancheng Exp $";

#include "uihor3dfrom2ddlg.h"

#include "array2dinterpol.h"
#include "uiarray2dinterpol.h"
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
    : uiDialog( p, Setup("Derive 3D Horizon","Specify parameters","104.0.5") )
    , hor2d_( h2d )
    , emserv_( ems )
    , hor3d_( 0 )
    , displayfld_( 0 )
    , copyfld_( 0 )		      
{
    copyfld_ = new uiCheckBox( this, "Copy grid only" );
    copyfld_->activated.notify( mCB(this,uiHor3DFrom2DDlg,copyCB) );

    interpolsel_ = new uiArray2DInterpolSel( this, false, false, false, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    interpolsel_->attach( alignedBelow, copyfld_ );

    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread = false;

    outfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    outfld_->attach( alignedBelow, interpolsel_ );
    if ( emserv_ )
    {
	displayfld_ = new uiCheckBox( this, "Display after generation" );
	displayfld_->attach( alignedBelow, outfld_ );
    }

    copyCB( 0 );
}


uiHor3DFrom2DDlg::~uiHor3DFrom2DDlg()
{
    if ( hor3d_ ) hor3d_->unRef();
}

void uiHor3DFrom2DDlg::copyCB( CallBacker* )
{
    interpolsel_->display( !copyfld_->isChecked() );
}


MultiID uiHor3DFrom2DDlg::getSelID() const
{
    return selid_;
}


bool uiHor3DFrom2DDlg::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
} 


bool uiHor3DFrom2DDlg::acceptOK( CallBacker* )
{
#define mErrRet(s) { uiMSG().error(s); return false; }

    if ( !copyfld_->isChecked() && !interpolsel_->acceptOK() )
	return false;

    PtrMan<IOObj> ioobj = outfld_->getIOObj( false );
    if ( !ioobj )
	return false;

    const BufferString typ = EM::Horizon3D::typeStr();

    const bool nameandtypeexist = ioobj && typ==ioobj->group();
    
    EM::EMManager& em = EM::EMM();

    if ( emserv_ && nameandtypeexist )
	emserv_->removeTreeObject( em.getObjectID(ioobj->key()) );
    
    const EM::ObjectID emobjid = em.createObject( typ, ioobj->name() );
    mDynamicCastGet(EM::Horizon3D*,hor3d,em.getObject(emobjid));
    if ( !hor3d )
	mErrRet( "Cannot create 3D horizon" );

    if ( hor3d_ )
	hor3d_->unRef();

    hor3d_ = hor3d;

    hor3d_->ref();
    hor3d_->setPreferredColor( hor2d_.preferredColor() );
    hor3d_->setMultiID( ioobj->key() );

    Array2DInterpol* interpolator = 
	copyfld_->isChecked() ? 0 : interpolsel_->getResult();
    if ( !copyfld_->isChecked() && !interpolator )
	mErrRet( "Cannot create interpolator" );

    //Takes over interpolator
    EM::Hor2DTo3D converter( hor2d_, interpolator, *hor3d_ );

    uiTaskRunner taskrunner( this );
    bool rv = taskrunner.execute( converter );

#undef mErrRet
    if ( !rv ) return false;

    PtrMan<Executor> exec = hor3d->saver();
    if ( !exec )
	return false;

    rv = taskrunner.execute( *exec );
    if ( rv )
	selid_ = ioobj->key();
    return rv;
}

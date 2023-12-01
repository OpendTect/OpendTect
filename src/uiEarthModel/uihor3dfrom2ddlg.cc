/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihor3dfrom2ddlg.h"

#include "array2dinterpol.h"
#include "emhor2dto3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uiempartserv.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiHor3DFrom2DDlg::uiHor3DFrom2DDlg( uiParent* p, const EM::Horizon2D& h2d,
				    uiEMPartServer* ems )
    : uiDialog( p, Setup(tr("Derive 3D Horizon"),mNoDlgTitle,
			 mODHelpKey(mHor3DFrom2DDlgHelpID)) )
    , hor2d_( h2d )
    , emserv_( ems )
    , hor3d_( 0 )
    , displayfld_( 0 )
{
    interpolsel_ = new uiArray2DInterpolSel( this, false, false, false, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? tr("[ft]") : tr("[m]") );

    outfld_ = new uiHorizon3DSel( this, false );
    outfld_->attach( alignedBelow, interpolsel_ );
    outfld_->setInputText( BufferString(h2d.name()," ","3D") );

    if ( emserv_ )
    {
	displayfld_ = new uiCheckBox( this, tr("Display after generation") );
	displayfld_->attach( alignedBelow, outfld_ );
    }
}


uiHor3DFrom2DDlg::~uiHor3DFrom2DDlg()
{
    if ( hor3d_ ) hor3d_->unRef();
}


MultiID uiHor3DFrom2DDlg::getSelID() const
{
    return selid_;
}


EM::Horizon3D* uiHor3DFrom2DDlg::getHor3D()
{
    return hor3d_;
}


bool uiHor3DFrom2DDlg::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


bool uiHor3DFrom2DDlg::acceptOK( CallBacker* )
{
#define mErrRet(s) { uiMSG().error(s); return false; }

    selid_ = MultiID::udf();
    if ( !interpolsel_->acceptOK() )
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
	mErrRet( toUiString("Cannot create 3D horizon") );

    if ( hor3d_ )
	hor3d_->unRef();

    hor3d_ = hor3d;

    hor3d_->ref();
    hor3d_->setPreferredColor( hor2d_.preferredColor() );
    hor3d_->setMultiID( ioobj->key() );

    Array2DInterpol* interpolator = interpolsel_->getResult();
    if ( !interpolator )
	mErrRet( toUiString("Cannot create interpolator") );

    uiTaskRunner taskrunner( this );
    //Takes over interpolator
    EM::Hor2DTo3D converter( hor2d_, interpolator, *hor3d_, &taskrunner );
    bool rv = TaskRunner::execute( &taskrunner, converter );

#undef mErrRet
    if ( !rv ) return false;

    PtrMan<Executor> exec = hor3d->saver();
    if ( !exec )
	return false;

    rv = TaskRunner::execute( &taskrunner, *exec );
    if ( rv )
    {
	selid_ = ioobj->key();
	BufferString source = hor2d_.multiID().toString();
	source.add( " (" ).add( hor2d_.name() ).add( ")" );
	ioobj->pars().update( sKey::CrFrom(), source );
	ioobj->updateCreationPars();
	IOM().commitChanges( *ioobj );
    }

    return rv;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          January 2007
________________________________________________________________________

-*/

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
#include "ioobjctxt.h"
#include "survinfo.h"
#include "od_helpids.h"


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

    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput(uiStrings::sHorizon(1)));
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


DBKey uiHor3DFrom2DDlg::getSelID() const
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


bool uiHor3DFrom2DDlg::acceptOK()
{
#define mErrRet(s) { uiMSG().error(s); return false; }

    if ( !interpolsel_->acceptOK() )
	return false;

    PtrMan<IOObj> ioobj = outfld_->getIOObj( false );
    if ( !ioobj )
	return false;

    const BufferString typ = EM::Horizon3D::typeStr();

    const bool nameandtypeexist = ioobj && typ==ioobj->group();

    if ( emserv_ && nameandtypeexist )
	emserv_->removeTreeObject( ioobj->key() );

    EM::Object* emobj = EM::Hor3DMan().createObject( typ, ioobj->name() );
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d )
	mErrRet( toUiString("Cannot create 3D horizon") );

    if ( hor3d_ )
	hor3d_->unRef();

    hor3d_ = hor3d;

    hor3d_->ref();
    hor3d_->setPreferredColor( hor2d_.preferredColor() );

    Array2DInterpol* interpolator = interpolsel_->getResult();
    if ( !interpolator )
	mErrRet( toUiString("Cannot create interpolator") );

    uiTaskRunnerProvider trprov( this );
    //Takes over interpolator
    EM::Hor2DTo3D converter( hor2d_, interpolator, *hor3d_, trprov );
    bool rv = trprov.execute( converter );

#undef mErrRet
    if ( !rv ) return false;

    PtrMan<Executor> exec = hor3d->saver();
    if ( !exec )
	return false;

    rv = trprov.execute( *exec );
    if ( rv )
    {
	selid_ = ioobj->key();
	BufferString source = hor2d_.dbKey().toString();
	source.add( " (" ).add( hor2d_.name() ).add( ")" );
	ioobj->pars().update( sKey::CrFrom(), source );
	ioobj->updateCreationPars();
	ioobj->commitChanges();
    }

    return rv;
}

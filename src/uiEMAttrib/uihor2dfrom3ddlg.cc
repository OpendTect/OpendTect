/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/

#include "uihor2dfrom3ddlg.h"

#include "bufstringset.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "hor2dfrom3dcreator.h"
#include "ptrman.h"
#include "posinfo.h"
#include "ioobj.h"
#include "seisioobjinfo.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiseispartserv.h"
#include "uitaskrunner.h"
#include "uibutton.h"
#include "od_helpids.h"


uiHor2DFrom3DDlg::uiHor2DFrom3DDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrCreate(tr("%1 from %2")
			  .arg(uiStrings::phrJoinStrings(uiStrings::s2D(),
			  uiStrings::sHorizon().toLower()))
			  .arg(uiStrings::s3D())), mNoDlgTitle,
			  mODHelpKey(mHor2DFrom3DDlgHelpID)))
{
    uiSurfaceRead::Setup srsu( "Horizon" );
    srsu.withattribfld( false );
    srsu.withsectionfld( false );
    hor3dsel_ = new uiSurfaceRead( this, srsu );

    linesetinpsel_ = new uiSeis2DMultiLineSel( this );
    linesetinpsel_->attach( alignedBelow, hor3dsel_ );

    out2dfld_ = new uiSurfaceWrite( this,
			uiSurfaceWrite::Setup(EM::Horizon2D::typeStr(),
					      EM::Horizon2D::userTypeStr() ) );

    out2dfld_->attach( alignedBelow, linesetinpsel_ );

    displayfld_ = new uiCheckBox( this, tr("Display on OK") );
    displayfld_->setChecked( true );
    displayfld_->attach( alignedBelow,out2dfld_ );
}


bool uiHor2DFrom3DDlg::acceptOK( CallBacker* )
{
    if ( !checkFlds() )
	return false;

    const MultiID mid = hor3dsel_->selIOObj()->key();
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( oid );
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	emobj = EM::EMM().createTempObject( hor3dsel_->selIOObj()->group() );
	if ( !emobj )
	{
	    uiMSG().error( tr("Cannot read or create 3D horizon") );
	    return false;
	}

	emobj->setMultiID( mid );
	PtrMan<Executor> loader = EM::EMM().objectLoader( mid );
	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute( &taskrunner, *loader ) )
	    return false;
    }

    const char* horizonnm = out2dfld_->getObjSel()->getInput();
    EM::Horizon2D* horizon2d = create2dHorizon( horizonnm );
    if ( !horizon2d )
	return false;

    horizon2d->ref();
    set2DHorizon( *horizon2d );

    PtrMan<Executor> saver = horizon2d->saver();
    uiTaskRunner writedlg( this );
    TaskRunner::execute( &writedlg, *saver );

    EM::EMObjectCallbackData cbdata;
    cbdata.event = EM::EMObjectCallbackData::PositionChange;
    saver = 0;
    if ( doDisplay() )
    {
	horizon2d->change.trigger( cbdata );
	horizon2d->unRefNoDelete();
    }
    else
	horizon2d->unRef();

    return true;
}


EM::Horizon2D* uiHor2DFrom3DDlg::create2dHorizon( const char* horizonnm )
{
    EM::EMManager& em = EM::EMM();
    emobjid_ = em.createObject( EM::Horizon2D::typeStr(), horizonnm );
    mDynamicCastGet( EM::Horizon2D*, horizon, em.getObject(emobjid_) );
    horizon->setMultiID( out2dfld_->selIOObj()->key() );
    return horizon;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiHor2DFrom3DDlg::checkFlds()
{
    if ( !hor3dsel_->getObjSel()->commitInput() )
	mErrRet( tr("Please select a valid 3D Horizon. ") )
    if ( !linesetinpsel_->nrSelected() )
	mErrRet( tr("Please select at least one 2D line") )
    if ( !out2dfld_->getObjSel()->commitInput() )
	mErrRet( tr("Enter the output Horizon where you want to write. ") )
    return true;
}


void uiHor2DFrom3DDlg::set2DHorizon( EM::Horizon2D& horizon2d )
{
    TypeSet<Pos::GeomID> geomids;
    linesetinpsel_->getSelGeomIDs( geomids );
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.getObjectID( hor3dsel_->selIOObj()->key() );
    mDynamicCastGet(EM::Horizon3D*,horizon3d,em.getObject(objid));
    Hor2DFrom3DCreatorGrp creator( *horizon3d, horizon2d );
    creator.init( geomids );

    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, creator );
}


bool uiHor2DFrom3DDlg::doDisplay() const
{
    return displayfld_->isChecked();
}

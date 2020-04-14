/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/

#include "uihor2dfrom3ddlg.h"

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
			  .arg(tr("2D Horizon"))
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


bool uiHor2DFrom3DDlg::acceptOK()
{
    if ( !checkFlds() )
	return false;

    const DBKey mid = hor3dsel_->selIOObj()->key();
    uiTaskRunnerProvider trprov( this );
    ConstRefMan<EM::Object> emobj = EM::Hor3DMan().fetch( mid, trprov );
    if ( !emobj )
	return false;

    emobj.setNoDelete( true );
    const char* horizonnm = out2dfld_->getObjSel()->getInput();
    EM::Horizon2D* horizon2d = create2dHorizon( horizonnm );
    if ( !horizon2d )
	return false;

    horizon2d->ref();
    set2DHorizon( *horizon2d );

    PtrMan<Executor> saver = horizon2d->saver();
    uiTaskRunner writedlg( this );
    TaskRunner::execute( &writedlg, *saver );

    EM::ObjectCallbackData cbdata( EM::Object::cPositionChange(),
				     Monitorable::ChangeData::cUnspecChgID() );
    saver = 0;
    if ( doDisplay() )
    {
	horizon2d->objectChanged().trigger( cbdata );
	horizon2d->unRefNoDelete();
    }
    else
	horizon2d->unRef();

    return true;
}


EM::Horizon2D* uiHor2DFrom3DDlg::create2dHorizon( const char* horizonnm )
{
    EM::Object* obj = EM::Hor2DMan().createObject(
				EM::Horizon2D::typeStr(), horizonnm );
    mDynamicCastGet( EM::Horizon2D*, horizon, obj );
    horizon->setDBKey( out2dfld_->selIOObj()->key() );
    emobjid_ = horizon->dbKey();
    return horizon;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiHor2DFrom3DDlg::checkFlds()
{
    if ( !hor3dsel_->getObjSel()->commitInput() )
	mErrRet( tr("Please select a valid 3D Horizon. ") )
    if ( !linesetinpsel_->nrSelected() )
	mErrRet( uiStrings::phrPlsSelectAtLeastOne( uiStrings::s2DLine() ) )
    if ( !out2dfld_->getObjSel()->commitInput() )
	mErrRet( tr("Enter the output Horizon where you want to write. ") )
    return true;
}


void uiHor2DFrom3DDlg::set2DHorizon( EM::Horizon2D& horizon2d )
{
    GeomIDSet geomids;
    linesetinpsel_->getSelGeomIDs( geomids );
    mDynamicCastGet(EM::Horizon3D*,horizon3d,
		    EM::Hor3DMan().getObject(hor3dsel_->selIOObj()->key()));
    Hor2DFrom3DCreatorGrp creator( *horizon3d, horizon2d );
    creator.init( geomids );

    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, creator );
}


bool uiHor2DFrom3DDlg::doDisplay() const
{
    return displayfld_->isChecked();
}

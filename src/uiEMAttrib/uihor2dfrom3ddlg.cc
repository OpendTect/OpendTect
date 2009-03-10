/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihor2dfrom3ddlg.cc,v 1.6 2009-03-10 12:29:08 cvsranojay Exp $";

#include "uihor2dfrom3ddlg.h"

#include "bufstringset.h"
#include "emmanager.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
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


uiHor2DFrom3DDlg::uiHor2DFrom3DDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create 2D horizon from 3D",
				 "Specify parameters","104.0.12"))
{
    uiSurfaceRead::Setup setup( "Horizon" );
    setup.withattribfld( false );
    setup.withsectionfld( false );
    hor3dsel_ = new uiSurfaceRead( this, setup );

    linesetinpsel_ = new uiSelection2DParSel( this );
    linesetinpsel_->attach( alignedBelow, hor3dsel_ );

    out2dfld_ = new uiSurfaceWrite( this, uiSurfaceWrite::Setup("2D Horizon") );
    out2dfld_->attach( alignedBelow, linesetinpsel_ );

    displayfld_ = new uiCheckBox( this, "Display on OK" );
    displayfld_->setChecked( true );
    displayfld_->attach( alignedBelow,out2dfld_ );
}


bool uiHor2DFrom3DDlg::acceptOK( CallBacker* )
{
    if ( !checkFlds() )
	return false;

    PtrMan<Executor> loader = EM::EMM().objectLoader(
	    			hor3dsel_->selIOObj()->key() );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    const char* horizonnm = out2dfld_->getObjSel()->getInput();
    EM::Horizon2D* horizon2d = create2dHorizon( horizonnm );
    if ( !horizon2d )
	return false;
    set2DHorizon( *horizon2d );
    horizon2d->ref();
    
    PtrMan<Executor> saver = horizon2d->saver();
    uiTaskRunner writedlg( this );
    writedlg.execute( *saver );

    saver = 0;
    if ( doDisplay() )
    {
	horizon2d->syncGeometry();
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
    if ( !hor3dsel_->getObjSel()->commitInput(false) )
	mErrRet( "Pease select a valid 3d Horizon. " )
    if ( linesetinpsel_->getSummary().isEmpty() )
	mErrRet( "Pease select a valid Lineset. " )
    if ( !out2dfld_->getObjSel()->commitInput(false) )
	mErrRet( "Enter the output surface where you want to write. " )
    return true;
}


void uiHor2DFrom3DDlg::set2DHorizon( EM::Horizon2D& horizon2d )
{
    const BufferStringSet sellinenames = linesetinpsel_->getSelLines();
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.getObjectID( hor3dsel_->selIOObj()->key() );
    mDynamicCastGet(EM::Horizon3D*,horizon3d,em.getObject(objid));
    for ( int idx=0; idx<sellinenames.size(); idx++ )
    {
	PosInfo::Line2DData posdata;

	uiSeisPartServer::get2DLineGeometry( linesetinpsel_->getIOObj()->key(),
					     sellinenames.get(idx), posdata );
	const int lineid =
	    horizon2d.geometry().addLine( linesetinpsel_->getIOObj()->key(),
				          sellinenames.get(idx).buf() );
	for ( int idy=0; idy<posdata.posns_.size(); idy++ )
	{
	    const PosInfo::Line2DPos& posinfo = posdata.posns_[idy];
	    BinID bid = SI().transform( posinfo.coord_ );
	    EM::SubID subid = bid.getSerialized();
	    const Coord3 pos3d = horizon3d->getPos( horizon3d->sectionID(0),
		    				    subid );
	    subid = RowCol( lineid, posinfo.nr_ ).getSerialized();
	    Coord3 pos( 0, 0, pos3d.z );
	    horizon2d.setPos(horizon2d.sectionID(0),subid,pos,false);
	}
    }
}

bool uiHor2DFrom3DDlg::doDisplay() const
{
    return displayfld_->isChecked(); 
}
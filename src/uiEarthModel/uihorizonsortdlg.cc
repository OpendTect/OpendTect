/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonsortdlg.cc,v 1.14 2009-03-24 12:33:51 cvsbert Exp $";

#include "uihorizonsortdlg.h"

#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "horizonsorter.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"


uiHorizonSortDlg::uiHorizonSortDlg( uiParent* p, bool is2d )
    : uiDialog(p,Setup("Horizon sorter","Select horizons",""))
    , ctio_(is2d ? *mMkCtxtIOObj(EMHorizon2D) : *mMkCtxtIOObj(EMHorizon3D))
{
    ioobjselgrp_ = new uiIOObjSelGrp( this, ctio_, "Select horizons", true );
    if ( ioobjselgrp_->getListField() )
	ioobjselgrp_->getListField()->setHSzPol( uiObject::WideVar );
}


uiHorizonSortDlg::~uiHorizonSortDlg()
{
    deepUnRef( horizons_ );
    delete &ctio_; delete ctio_.ioobj;
}


void uiHorizonSortDlg::setParConstraints( const IOPar& parconstraints,
					  bool includeconstraints,
					  bool allowcnstrsabsent )
{
    IOObjContext ctxt = ioobjselgrp_->getCtxtIOObj().ctxt;
    ctxt.parconstraints = parconstraints;
    ctxt.includeconstraints = includeconstraints;
    ctxt.allowcnstrsabsent = allowcnstrsabsent;
    ioobjselgrp_->setContext( ctxt );
}


void uiHorizonSortDlg::getSelectedHorizons( TypeSet<MultiID>& horids ) const
{
    for ( int idx=0; idx<ioobjselgrp_->nrSel(); idx++ )
	horids += ioobjselgrp_->selected( idx );
}


void uiHorizonSortDlg::getSortedHorizons( ObjectSet<EM::Horizon>& hors ) const
{
    hors = horizons_;
}


bool uiHorizonSortDlg::acceptOK( CallBacker* )
{
    if ( !ioobjselgrp_->processInput() )
    {
	uiMSG().error( "Invalid objects selected" );
	return false;
    }

    if ( ioobjselgrp_->nrSel() < 2 )
    {
	uiMSG().error( "Please select at least two horizons" );
	return false;
    }

    TypeSet<MultiID> horids;
    getSelectedHorizons( horids );

    TypeSet<MultiID> loadids;
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const EM::ObjectID oid = EM::EMM().getObjectID( horids[idx] );
	const EM::EMObject* emobj = EM::EMM().getObject(oid);
	if ( !emobj || !emobj->isFullyLoaded() )
	    loadids += horids[idx];
    }

    Executor* horreader = EM::EMM().objectLoader( loadids );
    if ( !horreader ) return false;

    ExecutorGroup execgrp("Reading horizons");
    execgrp.add( horreader );

    HorizonSorter* horsorter = new HorizonSorter( horids );
    execgrp.add( horsorter );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(execgrp) ) return false;

    horsorter->getSortedList( horids );
    deepUnRef( horizons_ );

    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const EM::ObjectID objid = EM::EMM().getObjectID( horids[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	emobj->ref();
	mDynamicCastGet(EM::Horizon*,horizon,emobj);
	if ( !horizon )
	    emobj->unRef();
	horizons_ += horizon;
    }

    bbox_.hrg = horsorter->getBoundingBox();
    return true;
}

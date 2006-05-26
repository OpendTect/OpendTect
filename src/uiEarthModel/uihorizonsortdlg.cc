/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2005
 RCS:		$Id: uihorizonsortdlg.cc,v 1.4 2006-05-26 08:13:33 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihorizonsortdlg.h"

#include "uicursor.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "horizonsorter.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"


uiHorizonSortDlg::uiHorizonSortDlg( uiParent* p )
    : uiDialog(p,Setup("Horizon sorter","Select horizons",""))
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( EMHorizon );
    ioobjselgrp_ = new uiIOObjSelGrp( this, *ctio, "Select horizons", true );
}


uiHorizonSortDlg::~uiHorizonSortDlg()
{
    deepUnRef( horizons_ );
}


void uiHorizonSortDlg::setParConstraints( const IOPar& parconstraints,
					  bool includeconstraints,
					  bool allowcnstrsabsent )
{
    CtxtIOObj ctio = ioobjselgrp_->getContext();
    ctio.ctxt.parconstraints = parconstraints;
    ctio.ctxt.includeconstraints = includeconstraints;
    ctio.ctxt.allowcnstrsabsent = allowcnstrsabsent;
    ioobjselgrp_->setContext( ctio );
}


void uiHorizonSortDlg::getSelectedHorizons( TypeSet<MultiID>& horids ) const
{
    for ( int idx=0; idx<ioobjselgrp_->nrSel(); idx++ )
    {
	const IOObj* ioobj = ioobjselgrp_->selected( idx );
	if ( !ioobj ) continue;

	const MultiID mid = ioobj->key();
	horids += mid;
    }
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
	uiMSG().error( "Please seclect at least two horizons" );
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
	    loadids +=  horids[idx];
    }

    Executor* horreader = EM::EMM().objectLoader( loadids );
    if ( !horreader ) return false;

    ExecutorGroup execgrp("Reading horizons");
    execgrp.add( horreader );

    HorizonSorter* horsorter = new HorizonSorter( horids );
    execgrp.add( horsorter );

    uiExecutor dlg( this, execgrp );
    if ( !dlg.go() ) return false;

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

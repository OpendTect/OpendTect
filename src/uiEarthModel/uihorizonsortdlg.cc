/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2005
 RCS:		$Id: uihorizonsortdlg.cc,v 1.1 2006-04-28 15:22:48 cvsnanne Exp $
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

    PtrMan<Executor> horreader = EM::EMM().objectLoader( horids );
    if ( !horreader ) return false;
    horreader->setName( "Reading horizons" );
    uiExecutor dlg( this, *horreader );
    if ( !dlg.go() ) return false;

    HorizonSorter sorter( horids );
    uiExecutor sortdlg( this, sorter );
    if ( !sortdlg.go() ) return false;

    sorter.getSortedList( horids );
    horizons_.erase();
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const EM::ObjectID objid = EM::EMM().getObjectID( horids[idx] );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	emobj->ref();
	mDynamicCastGet(EM::Horizon*,horizon,emobj);
	if ( !horizon )
	    emobj->unRef();
	else
	    emobj->unRefNoDelete();
	horizons_ += horizon;
    }

    bbox_.hrg = sorter.getBoundingBox();
    return true;
}

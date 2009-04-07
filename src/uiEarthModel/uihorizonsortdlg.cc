/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonsortdlg.cc,v 1.15 2009-04-07 07:12:20 cvssatyaki Exp $";

#include "uihorizonsortdlg.h"

#include "uisurfacesel.h"
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
    , is2d_( is2d )
{
    if ( is2d )
	horsel_ = new uiHorizon2DSel( this );
    else
	horsel_ = new uiHorizon3DSel( this );

}


uiHorizonSortDlg::~uiHorizonSortDlg()
{ deepUnRef( horizons_ ); }


void uiHorizonSortDlg::setLineID( const MultiID& mid )
{
    if ( is2d_ )
    {
	mDynamicCastGet( uiSurface2DSel*, s2dsel, horsel_ ); 
	s2dsel->setLineSetID( mid );
    }
}


void uiHorizonSortDlg::getSelectedHorizons( TypeSet<MultiID>& horids ) const
{
    horsel_->getSelSurfaceIds( horids );
}


void uiHorizonSortDlg::getSortedHorizons( ObjectSet<EM::Horizon>& hors ) const
{
    hors = horizons_;
}


bool uiHorizonSortDlg::acceptOK( CallBacker* )
{
    if ( horsel_->getSelItems() < 2 )
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

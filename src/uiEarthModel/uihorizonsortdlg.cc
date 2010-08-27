/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonsortdlg.cc,v 1.20 2010-08-27 05:01:00 cvsraman Exp $";

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
#include "horizonrelation.h"
#include "horizonsorter.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"


uiHorizonSortDlg::uiHorizonSortDlg( uiParent* p, bool is2d, bool loadneeded )
    : uiDialog(p,Setup("Horizon sorter","Select horizons",""))
    , is2d_( is2d )
    , loadneeded_(loadneeded)
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


void uiHorizonSortDlg::setConstSelected( const TypeSet<MultiID>& horids )
{
    constselids_ = horids;
    horsel_->removeFromList( horids );
}


void uiHorizonSortDlg::getSelectedHorizons( TypeSet<MultiID>& horids ) const
{
    horsel_->getSelSurfaceIds( horids );
    horids.append( constselids_ );
}


void uiHorizonSortDlg::getSortedHorizons( ObjectSet<EM::Horizon>& hors ) const
{
    hors = horizons_;
}


void uiHorizonSortDlg::getSortedHorizonIDs( TypeSet<MultiID>& horids ) const
{
    horids = horids_;
}


bool uiHorizonSortDlg::acceptOK( CallBacker* )
{
    TypeSet<MultiID> horids;
    getSelectedHorizons( horids );
    if ( horids.size() < 2 )
    {
	uiMSG().error( "Please select at least two horizons" );
	return false;
    }

    if ( !loadneeded_ && sortFromRelationTree(horids) )
	return true;

    TypeSet<MultiID> loadids;
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	const EM::ObjectID oid = EM::EMM().getObjectID( horids[idx] );
	const EM::EMObject* emobj = EM::EMM().getObject(oid);
	if ( !emobj || !emobj->isFullyLoaded() )
	    loadids += horids[idx];
    }

    ExecutorGroup execgrp("Reading horizons");

    Executor* horreader = EM::EMM().objectLoader( loadids );
    if ( horreader )
	execgrp.add( horreader );

    HorizonSorter* horsorter = new HorizonSorter( horids, is2d_ );
    execgrp.add( horsorter );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(execgrp) ) return false;

    horsorter->getSortedList( horids_ );
    updateRelationTree( horids_ );
    bbox_.hrg = horsorter->getBoundingBox();
    if ( !loadneeded_ )
	return true;

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

    return true;
}


bool uiHorizonSortDlg::sortFromRelationTree( const TypeSet<MultiID>& ids )
{
    EM::RelationTree reltree( is2d_ );
    TypeSet<MultiID> sortedids;
    reltree.getSorted( ids, sortedids );
    if ( sortedids.size() == ids.size() )
    {
	horids_ = sortedids;
	return true;
    }

    return false;
}


void uiHorizonSortDlg::updateRelationTree( const TypeSet<MultiID>& ids )
{
    if ( ids.size() < 2 )
	return;

    EM::RelationTree reltree( is2d_ );
    for ( int idx=1; idx<ids.size(); idx++ )
	reltree.addRelation( ids[idx-1], ids[idx], false );

    reltree.write();
}


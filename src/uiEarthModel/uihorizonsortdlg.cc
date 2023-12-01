/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorizonsortdlg.h"

#include "uisurfacesel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "horizonrelation.h"
#include "horizonsorter.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"


uiHorizonSortDlg::uiHorizonSortDlg( uiParent* p, bool is2d, bool loadneeded )
    : uiDialog(p,Setup(tr("Select Horizons"),mNoDlgTitle,mNoHelpKey))
    , is2d_( is2d )
    , loadneeded_(loadneeded)
{
    if ( is2d )
	horsel_ = new uiHorizon2DSelGrp( this );
    else
	horsel_ = new uiHorizon3DSelGrp( this );

}


uiHorizonSortDlg::~uiHorizonSortDlg()
{
    deepUnRef( horizons_ );
}


void uiHorizonSortDlg::setSelected( const TypeSet<MultiID>& horids )
{
    horsel_->setChosen( horids );
}


void uiHorizonSortDlg::getSelectedHorizons( TypeSet<MultiID>& horids ) const
{
    horsel_->getChosen( horids );
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
	uiMSG().error( tr("Please select at least two horizons") );
	return false;
    }

    const bool sorted = sortFromRelationTree( horids );
    uiTaskRunner taskrunner( this );
    PtrMan<Executor> horreader = 0;
    if ( !sorted || loadneeded_ )
    {
	TypeSet<MultiID> loadids;
	for ( int idx=0; idx<horids.size(); idx++ )
	{
	    const EM::ObjectID oid = EM::EMM().getObjectID( horids[idx] );
	    const EM::EMObject* emobj = EM::EMM().getObject(oid);
	    if ( !emobj || !emobj->isFullyLoaded() )
		loadids += horids[idx];
	}

	horreader = EM::EMM().objectLoader( loadids );
	if ( horreader && !TaskRunner::execute( &taskrunner, *horreader ) )
	    return false;
    }

    PtrMan<HorizonSorter> horsorter = 0;
    if ( sorted )
    {
	for ( int idx=0; idx<horids.size(); idx++ )
	{
	    EM::IOObjInfo oi( horids[idx] ); EM::SurfaceIOData sd;
	    uiString errmsg;
	    if ( !oi.getSurfaceData(sd,errmsg)  )
	    { uiMSG().error(errmsg); return false; }

	    if ( !idx )
		bbox_.hsamp_ = sd.rg;
	    else
	    {
		bbox_.hsamp_.include( sd.rg.start_);
		bbox_.hsamp_.include( sd.rg.stop_);
	    }
	}
    }
    else
    {
	horsorter = new HorizonSorter( horids, is2d_ );
	if ( !TaskRunner::execute( &taskrunner, *horsorter ) ) return false;

	horsorter->getSortedList( horids_ );
	updateRelationTree( horids_ );
	bbox_.hsamp_ = horsorter->getBoundingBox();
    }

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
    if ( sortedids.size() != ids.size() )
	return false;

    horids_ = sortedids;
    return true;
}


void uiHorizonSortDlg::updateRelationTree( const TypeSet<MultiID>& ids )
{
    if ( ids.size() < 2 )
	return;

    EM::RelationTree::update( is2d_, ids );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2005
________________________________________________________________________

-*/

#include "uihorizonsortdlg.h"

#include "uisurfacesel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "ioobjctxt.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "horizonrelation.h"
#include "horizonsorter.h"
#include "ioobj.h"
#include "iopar.h"
#include "mousecursor.h"
#include "ptrman.h"


uiHorizonSortDlg::uiHorizonSortDlg( uiParent* p, bool is2d, bool loadneeded )
    : uiDialog(p,Setup(tr("Horizon sorter"),tr("Select horizons"),mNoHelpKey))
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


void uiHorizonSortDlg::setConstSelected( const DBKeySet& horids )
{
    constselids_ = horids;
    horsel_->removeFromList( horids );
}


void uiHorizonSortDlg::getSelectedHorizons( DBKeySet& horids ) const
{
    horsel_->getSelSurfaceIds( horids );
    horids.append( constselids_ );
}


void uiHorizonSortDlg::getSortedHorizons( ObjectSet<EM::Horizon>& hors ) const
{
    hors = horizons_;
}


void uiHorizonSortDlg::getSortedHorizonIDs( DBKeySet& horids ) const
{
    horids = horids_;
}


bool uiHorizonSortDlg::acceptOK()
{
    DBKeySet horids;
    getSelectedHorizons( horids );
    if ( horids.size() < 2 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("at least two horizons")) );
	return false;
    }

    bool sorted = sortFromRelationTree( horids );
    uiTaskRunnerProvider trprov( this );
    EM::ObjectManager& emman = is2d_ ? EM::Hor2DMan() : EM::Hor3DMan();
    RefObjectSet<EM::Object> emobjs = emman.loadObjects( horids, trprov );
    if ( emobjs.size() != horids.size() )
	return false;

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
	horsorter = new HorizonSorter( horids, trprov, is2d_ );
	if ( !horsorter->execute() ) return false;

	horsorter->getSortedList( horids_ );
	updateRelationTree( horids_ );
	bbox_.hsamp_ = horsorter->getBoundingBox();
    }

    if ( !loadneeded_ )
	return true;

    deepUnRef( horizons_ );

    for ( int idx=0; idx<emobjs.size(); idx++ )
    {
	mDynamicCastGet(EM::Horizon*,horizon,emobjs[idx]);
	if ( horizon )
	    horizon->ref();

	horizons_ += horizon;
    }

    return true;
}


bool uiHorizonSortDlg::sortFromRelationTree( const DBKeySet& ids )
{
    EM::RelationTree reltree( is2d_ );
    DBKeySet sortedids;
    reltree.getSorted( ids, sortedids );
    if ( sortedids.size() != ids.size() )
	return false;

    horids_ = sortedids;
    return true;
}


void uiHorizonSortDlg::updateRelationTree( const DBKeySet& ids )
{
    if ( ids.size() < 2 )
	return;

    EM::RelationTree::update( is2d_, ids );
}

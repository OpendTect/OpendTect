/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "psviewer2dgatherpainter.h"

#include "prestackgather.h"
#include "flatview.h"

namespace PreStackView
{


Viewer2DGatherPainter::Viewer2DGatherPainter( FlatView::Viewer& v )
    : inputgather_( 0 )
    , viewer_( v )
{}


Viewer2DGatherPainter::~Viewer2DGatherPainter()
{
    if ( inputgather_ ) DPM( DataPackMgr::FlatID() ).release(inputgather_->id());
}


BinID Viewer2DGatherPainter::getBinID() const
{ return inputgather_ ? inputgather_->getBinID() : BinID(-1,-1); }


void Viewer2DGatherPainter::setGather( DataPack::ID id )
{
    if ( inputgather_ && inputgather_->id()==id )
	return;

    const bool hadwvadata = viewer_.packID( true )!=DataPack::cNoID();
    const bool hadvddata = viewer_.packID( false )!=DataPack::cNoID();
    const bool haddata = inputgather_;
    if ( inputgather_ )
    {
	viewer_.removePack( inputgather_->id() );
	DPM( DataPackMgr::FlatID() ).release( inputgather_->id() );
	inputgather_ = 0;
    }

    DataPack* dp = DPM( DataPackMgr::FlatID() ).obtain( id, false );
    mDynamicCastGet( PreStack::Gather*, gather, dp );
    if ( gather ) inputgather_ = gather;
    else if ( dp ) DPM( DataPackMgr::FlatID() ).release( dp->id() );
    if ( inputgather_ )
    {
	viewer_.addPack( id, true );
	if ( hadwvadata )
	    viewer_.usePack( true, id, !haddata );

	if ( !haddata || hadvddata )
	    viewer_.usePack( false, id, !haddata );
    }
}


}; //namespace

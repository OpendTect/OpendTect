/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "psviewer2dgatherpainter.h"

#include "prestackgather.h"
#include "flatview.h"

namespace PreStackView
{


Viewer2DGatherPainter::Viewer2DGatherPainter( FlatView::Viewer& v )
    : inputwvagather_( 0 )
    , inputvdgather_( 0 )
    , viewer_( v )
{}


Viewer2DGatherPainter::~Viewer2DGatherPainter()
{
    if ( inputwvagather_ )
	DPM( DataPackMgr::FlatID() ).release(inputwvagather_->id());
    if ( inputvdgather_ )
	DPM( DataPackMgr::FlatID() ).release(inputvdgather_->id());
}


BinID Viewer2DGatherPainter::getBinID() const
{ return inputwvagather_ ? inputwvagather_->getBinID()
    			 : inputvdgather_ ? inputvdgather_->getBinID()
			     		  : BinID(-1,-1); }


void Viewer2DGatherPainter::setVDGather( DataPack::ID vdid )
{
    if ( inputvdgather_ && inputvdgather_->id()==vdid ) 
	return;

    const bool hadpack = inputvdgather_;
    if ( inputvdgather_ )
    {
	viewer_.removePack( inputvdgather_->id() );
	DPM( DataPackMgr::FlatID() ).release( inputvdgather_->id() );
	inputvdgather_ = 0;
    }

    DataPack* vddp = DPM( DataPackMgr::FlatID() ).obtain( vdid );
    mDynamicCastGet( PreStack::Gather*, vdgather, vddp );
    viewer_.appearance().ddpars_.vd_.show_ = vdgather;
    Interval<float> rng;
    viewer_.appearance().ddpars_.vd_.mappersetup_.range_ = rng.udf();
    if ( vdgather )
    {
	inputvdgather_ = vdgather;
	viewer_.setPack( false, vdid, !hadpack );
    }
    else if ( vddp )
	DPM( DataPackMgr::FlatID() ).release( vddp->id() );
}



void Viewer2DGatherPainter::setWVAGather( DataPack::ID wvaid )
{
    if ( wvaid<0 &&inputwvagather_ && inputwvagather_->id()==wvaid )
	return;

    const bool hadpack = inputwvagather_;
    if ( inputwvagather_ )
    {
	viewer_.removePack( inputwvagather_->id() );
	DPM( DataPackMgr::FlatID() ).release( inputwvagather_->id() );
	inputwvagather_ = 0;
    }

    DataPack* wvadp = DPM( DataPackMgr::FlatID() ).obtain( wvaid );
    mDynamicCastGet( PreStack::Gather*, wvagather, wvadp );
    viewer_.appearance().ddpars_.wva_.show_ = wvagather;
    if ( wvagather )
    {
	inputwvagather_ = wvagather;
	viewer_.setPack( true, wvaid, !hadpack );
    }
    else if ( wvadp )
	DPM( DataPackMgr::FlatID() ).release( wvadp->id() );
}


}; //namespace

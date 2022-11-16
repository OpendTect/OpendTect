/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
	DPM( DataPackMgr::FlatID() ).unRef(inputwvagather_->id());
    if ( inputvdgather_ )
	DPM( DataPackMgr::FlatID() ).unRef(inputvdgather_->id());
}


BinID Viewer2DGatherPainter::getBinID() const
{
    return inputwvagather_ ? inputwvagather_->getBinID()
			   : (inputvdgather_ ? inputvdgather_->getBinID()
					     : BinID::udf());
}


void Viewer2DGatherPainter::setVDGather( DataPackID vdid )
{
    if ( inputvdgather_ && inputvdgather_->id()==vdid )
	return;

    const bool hadpack = inputvdgather_;
    if ( inputvdgather_ )
    {
	viewer_.removePack( inputvdgather_->id() );
	DPM( DataPackMgr::FlatID() ).unRef( inputvdgather_->id() );
	inputvdgather_ = nullptr;
    }

    auto vdgather = DPM( DataPackMgr::FlatID() ).get<PreStack::Gather>( vdid );
    viewer_.appearance().ddpars_.vd_.show_ = vdgather;
    Interval<float> rng;
    viewer_.appearance().ddpars_.vd_.mappersetup_.range_ = rng.udf();
    if ( vdgather )
    {
	inputvdgather_ = vdgather;
	viewer_.setPack( FlatView::Viewer::VD, vdid, !hadpack );
    }
}



void Viewer2DGatherPainter::setWVAGather( DataPackID wvaid )
{
    if ( !wvaid.isValid() &&inputwvagather_ && inputwvagather_->id()==wvaid )
	return;

    const bool hadpack = inputwvagather_;
    if ( inputwvagather_ )
    {
	viewer_.removePack( inputwvagather_->id() );
	DPM( DataPackMgr::FlatID() ).unRef( inputwvagather_->id() );
	inputwvagather_ = nullptr;
    }

    auto wvagather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( wvaid );
    viewer_.appearance().ddpars_.wva_.show_ = wvagather;
    if ( wvagather )
    {
	inputwvagather_ = wvagather;
	viewer_.setPack( FlatView::Viewer::WVA, wvaid, !hadpack );
    }
}

} // namespace PreStackView

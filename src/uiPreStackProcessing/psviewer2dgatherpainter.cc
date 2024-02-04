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
    : viewer_(v)
{
}


Viewer2DGatherPainter::~Viewer2DGatherPainter()
{
}


TrcKey Viewer2DGatherPainter::getTrcKey() const
{
    return inputwvagather_ ? inputwvagather_->getTrcKey()
			   : (inputvdgather_ ? inputvdgather_->getTrcKey()
					     : TrcKey::udf());
}


void Viewer2DGatherPainter::setVDGather( const PreStack::Gather* vddp )
{
    if ( inputvdgather_ && inputvdgather_.ptr()==vddp )
	return;

    const bool hadpack = inputvdgather_;
    if ( inputvdgather_ )
    {
	viewer_.removePack( FlatView::Viewer::VD );
	inputvdgather_ = nullptr;
    }

    viewer_.appearance().ddpars_.vd_.show_ = vddp;
    Interval<float> rng;
    viewer_.appearance().ddpars_.vd_.mappersetup_.range_ = rng.udf();
    if ( vddp )
    {
	inputvdgather_ = vddp;
	viewer_.setPack( FlatView::Viewer::VD,
			 const_cast<PreStack::Gather*>(vddp), !hadpack );
    }
}



void Viewer2DGatherPainter::setWVAGather( const PreStack::Gather* wvagather )
{
    if ( inputwvagather_ && inputwvagather_.ptr()==wvagather )
	return;

    const bool hadpack = inputwvagather_;
    if ( inputwvagather_ )
    {
	viewer_.removePack( FlatView::Viewer::WVA );
	inputwvagather_ = nullptr;
    }

    viewer_.appearance().ddpars_.wva_.show_ = wvagather;
    if ( wvagather )
    {
	inputwvagather_ = wvagather;
	viewer_.setPack( FlatView::Viewer::WVA,
			 const_cast<PreStack::Gather*>(wvagather), !hadpack );
    }
}


void Viewer2DGatherPainter::setVDGather( DataPackID vdid )
{
    if ( inputvdgather_ && inputvdgather_->id()==vdid )
	return;

     const bool hadpack = inputvdgather_;
     if ( inputvdgather_ )
     {
	viewer_.removePack( FlatView::Viewer::VD );
	inputvdgather_ = nullptr;
     }

    auto vdgather = DPM( DataPackMgr::FlatID() ).get<PreStack::Gather>( vdid );
    viewer_.appearance().ddpars_.vd_.show_ = vdgather;
    Interval<float> rng;
    viewer_.appearance().ddpars_.vd_.mappersetup_.range_ = rng.udf();
    if ( vdgather )
    {
	inputvdgather_ = vdgather;
	viewer_.setPack( FlatView::Viewer::VD, vdgather, !hadpack );
    }
}


void Viewer2DGatherPainter::setWVAGather( DataPackID wvaid )
{
    if ( !wvaid.isValid() &&inputwvagather_ && inputwvagather_->id()==wvaid )
	return;

    const bool hadpack = inputwvagather_;
    if ( inputwvagather_ )
    {
	viewer_.removePack( FlatView::Viewer::WVA );
	inputwvagather_ = nullptr;
    }

    auto wvagather = DPM(DataPackMgr::FlatID()).get<PreStack::Gather>( wvaid );
    viewer_.appearance().ddpars_.wva_.show_ = wvagather;
    if ( wvagather )
    {
	inputwvagather_ = wvagather;
	viewer_.setPack( FlatView::Viewer::WVA, wvagather, !hadpack );
    }
}

} // namespace PreStackView

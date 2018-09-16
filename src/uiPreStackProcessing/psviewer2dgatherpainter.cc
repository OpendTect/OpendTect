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
{}


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
	viewer_.removePack( inputvdgather_->id() );

    inputvdgather_ = DPM( DataPackMgr::FlatID() ).get<Gather>( vdid );
    viewer_.appearance().ddpars_.vd_.show_ = inputvdgather_;
    if ( inputvdgather_ )
        viewer_.setPack( false, vdid, !hadpack );
}



void Viewer2DGatherPainter::setWVAGather( DataPack::ID wvaid )
{
    if ( wvaid.isInvalid() &&inputwvagather_ && inputwvagather_->id()==wvaid )
	return;

    const bool hadpack = inputwvagather_;
    if ( inputwvagather_ )
	viewer_.removePack( inputwvagather_->id() );

    inputwvagather_ = DPM( DataPackMgr::FlatID() ).get<Gather>( wvaid );

    viewer_.appearance().ddpars_.wva_.show_ = inputwvagather_;

    if ( inputwvagather_ )
	viewer_.setPack( true, wvaid, !hadpack );
}


}; //namespace

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoupdater.h"

#include <vsg/app/Viewer.h>

#include <algorithm>

namespace vsgGeo
{

vsg::ref_ptr<Updater> Updater::install( vsg::Viewer& viewer )
{
    vsg::ref_ptr<Updater> updater = Updater::create( viewer );
    viewer.addUpdateOperation( updater, vsg::UpdateOperations::ALL_FRAMES );
    return updater;
}


Updater::Updater( vsg::Viewer& viewer )
    : viewer_(&viewer)
    , compiler_(Compiler::create())
    , options_(vsg::Options::create())
{
    options_->sharedObjects = vsg::SharedObjects::create();
}


Updater::~Updater()
{}


void Updater::requestUpdate( vsg::ref_ptr<Updatable> obj )
{
    if ( !obj )
	return;

    std::scoped_lock lock( lock_ );
    if ( std::find(updates_.begin(),updates_.end(),obj) == updates_.end() )
	updates_.push_back( obj );
}


size_t Updater::nrPendingUpdates() const
{
    std::scoped_lock lock( lock_ );
    return updates_.size();
}


void Updater::run()
{
    vsg::ref_ptr<vsg::Viewer> viewer = viewer_.ref_ptr();
    if ( !viewer )
	return;

    std::vector<vsg::ref_ptr<Updatable>> updates;
    {
	std::scoped_lock lock( lock_ );
	updates.swap( updates_ );
    }

    for ( auto& obj : updates )
	obj->applyUpdates( *viewer, *compiler_ );

    compiler_->process( *viewer );
}

} // namespace vsgGeo

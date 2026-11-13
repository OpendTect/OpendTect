/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeocompiler.h"

#include <vsg/app/CompileManager.h>
#include <vsg/app/Viewer.h>
#include <vsg/io/Logger.h>
#include <vsg/nodes/Group.h>
#include <vsg/ui/FrameStamp.h>
#include <algorithm>

namespace vsgGeo
{

Compiler::Compiler()
{}


Compiler::~Compiler()
{}


void Compiler::add( vsg::ref_ptr<vsg::Object> obj )
{
    if ( !obj )
	return;

    std::scoped_lock lock( lock_ );
    tocompile_.push_back( obj );
}


void Compiler::release( vsg::ref_ptr<vsg::Object> obj )
{
    if ( !obj )
	return;

    std::scoped_lock lock( lock_ );
    retired_.push_back( Retired{lastframe_, obj} );
}


void Compiler::setReleaseDelay( uint64_t nrframes )
{
    std::scoped_lock lock( lock_ );
    releasedelay_ = nrframes;
}


size_t Compiler::nrPendingCompiles() const
{
    std::scoped_lock lock( lock_ );
    return tocompile_.size();
}


size_t Compiler::nrPendingReleases() const
{
    std::scoped_lock lock( lock_ );
    return retired_.size();
}


void Compiler::process( vsg::Viewer& viewer )
{
    std::vector<vsg::ref_ptr<vsg::Object>> tocompile;
    std::vector<vsg::ref_ptr<vsg::Object>> todelete;
    {
	std::scoped_lock lock( lock_ );
	const vsg::FrameStamp* fs = viewer.getFrameStamp();
	lastframe_ = fs ? fs->frameCount : 0;

	if ( viewer.compileManager )
	    tocompile.swap( tocompile_ );

	while ( !retired_.empty() &&
		retired_.front().frame_ + releasedelay_ <= lastframe_ )
	{
	    todelete.push_back( retired_.front().obj_ );
	    retired_.pop_front();
	}
    }

    std::vector<vsg::ref_ptr<vsg::Object>> single;
    vsg::ref_ptr<vsg::Group> batch = vsg::Group::create();
    for ( auto& obj : tocompile )
    {
	vsg::ref_ptr<vsg::Node> node( obj.cast<vsg::Node>() );
	if ( node )
	    batch->addChild( node );
	else
	    single.push_back( obj );
    }

    if ( batch->children.size() > 1 && compileObject(viewer,batch) )
	batch->children.clear();

    for ( auto& child : batch->children )
	single.push_back( child );

    for ( auto& obj : single )
    {
	if ( !compileObject(viewer,obj) )
	    nrfailed_++;
    }

    todelete.clear();
}


static std::vector<vsg::observer_ptr<vsg::Viewer>>& drawingViewers()
{
    static std::vector<vsg::observer_ptr<vsg::Viewer>> viewers;
    return viewers;
}


void Compiler::addViewer( vsg::Viewer* viewer )
{
    if ( !viewer )
	return;

    for ( const auto& obs : drawingViewers() )
    {
	if ( obs.ref_ptr().get() == viewer )
	    return;
    }

    drawingViewers().emplace_back( viewer );
}


void Compiler::removeViewer( vsg::Viewer* viewer )
{
    auto& viewers = drawingViewers();
    viewers.erase(std::remove_if(viewers.begin(), viewers.end(),
	[viewer](const vsg::observer_ptr<vsg::Viewer>& obs)
	{
	    const vsg::ref_ptr<vsg::Viewer> vwr = obs.ref_ptr();
	    return !vwr || vwr.get() == viewer;
	}), viewers.end());
}


bool Compiler::compileObject( vsg::Viewer& viewer, vsg::ref_ptr<vsg::Object> obj )
{
    std::vector<vsg::ref_ptr<vsg::Viewer>> targets;
    for ( const auto& obs : drawingViewers() )
    {
	vsg::ref_ptr<vsg::Viewer> vwr = obs.ref_ptr();
	if ( vwr && vwr->compileManager )
	    targets.push_back(vwr);
    }

    if ( targets.empty() )	// tests: only the common viewer
	targets.push_back( vsg::ref_ptr<vsg::Viewer>(&viewer) );

    bool allok = true;
    for ( auto& vwr : targets )
    {
	const vsg::CompileResult result = vwr->compileManager->compile(obj);
	if ( !result )
	{
	    vsg::warn( "vsgGeo::Compiler: compile failed: ", result.message );
	    allok = false;
	    continue;
	}

	vsg::updateViewer( *vwr, result );
    }

    return allok;
}


void Compiler::releaseAll()
{
    std::deque<Retired> todelete;
    {
	std::scoped_lock lock( lock_ );
	todelete.swap( retired_ );
    }
}

} // namespace vsgGeo

/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "emeditor.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emtracker.h"
#include "geeditor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "rcollinebuilder.h"

namespace MPE
{

mImplClassFactory( ObjectEditor, factory );


ObjectEditor::ObjectEditor( EM::Object& emobj )
    : emobject_(&emobj)
    , editpositionchange(this)
    , movingnode_( EM::PosID::getInvalid() )
    , snapafteredit_( true )
    , nrusers_(0)
    , geeditor_(0)
{}


ObjectEditor::~ObjectEditor()
{
    CallBack::removeFromThreadCalls( this );
    delete geeditor_;
}


static bool nodecloningenabled = false;
static int nodeclonecountdown = -1;

void ObjectEditor::enableNodeCloning( bool yn )
{ nodecloningenabled = yn; }


void ObjectEditor::startEdit(const EM::PosID& pid)
{
    changedpids_.erase();

    alongmovingnodes_.erase();
    alongmovingnodesfactors_.erase();
    alongmovingnodesstart_.erase();
    snapafterthisedit_ = false;

    movingnode_ = pid;

    startpos_ = getPosition(movingnode_);
    if ( !startpos_.isDefined() )
    {
	pErrMsg( "Editnode is undefined");
	movingnode_ = EM::PosID::getInvalid();
	return;
    }

    getAlongMovingNodes( pid, alongmovingnodes_, &alongmovingnodesfactors_ );
    snapafterthisedit_ = snapafteredit_ && canSnapAfterEdit(pid);

    alongmovingnodesstart_.erase();
    for ( int idx=0; idx<alongmovingnodes_.size(); idx++ )
	alongmovingnodesstart_ += emobject_->getPos(alongmovingnodes_[idx]);

    nodeclonecountdown = nodecloningenabled ? 3 : -1;
}


bool ObjectEditor::setPosition(const Coord3& np)
{
    if ( movingnode_.isInvalid() )
    {
	pErrMsg("Moving unknown node");
	return false;
    }

    if ( !np.isDefined() )
    {
	pErrMsg("You cannot set the editnode to undefined");
	return false;
    }

    const Coord3 diff = np-startpos_;
    if ( !setPosition( movingnode_, np ) )
	return false;

    for ( int idx=0; idx<alongmovingnodes_.size(); idx++ )
    {
	const Coord3 newpos = alongmovingnodesstart_[idx] +
		    alongmovingnodesfactors_[idx]*diff;
	if ( !setPosition( alongmovingnodes_[idx], newpos ) )
	    return false;
    }

    nodeclonecountdown--;
    if ( !nodeclonecountdown )
	mMainThreadCall( ObjectEditor::cloneMovingNode );

    return true;
}


void ObjectEditor::finishEdit()
{
    if ( changedpids_.isEmpty() )
	return;

    if ( snapafterthisedit_ )
    {
//	const int trackeridx = MPE::engine().getTrackerByObject(emobject.id());
//	EMTracker* tracker = MPE::engine().getTracker(trackeridx);
//	tracker->snapPositions(alongmovingnodes);
    }

    /*emobject_->getMgr().undo().setUserInteractionEnd(
	    emobject_->getMgr().undo().currentEventID() );*/
}


bool ObjectEditor::canSnapAfterEdit(const EM::PosID& pid) const
{
    if ( MPE::engine().getTrackerByObject(emobject_->id())==-1 )
	return false;

    const TrcKeyZSampling& trackvolume = MPE::engine().activeVolume();

    TypeSet<EM::PosID> nodes;
    getAlongMovingNodes( pid, nodes, 0 );
    for ( int idx=0; idx<nodes.size(); idx++ )
    {
	const Coord3 pos = emobject_->getPos(nodes[idx]);
	const BinID bid = SI().transform(pos.getXY());

	if ( !trackvolume.hsamp_.includes( bid ) )
	    return false;
	if ( !trackvolume.zsamp_.includes( pos.z_,false ) )
	    return false;
    }

    return true;
}


bool ObjectEditor::getSnapAfterEdit() const { return snapafteredit_; }


void ObjectEditor::setSnapAfterEdit(bool yn) { snapafteredit_=yn; }


void ObjectEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();
    const Geometry::ElementEditor* ge = getEditor();
    if ( ge )
	ge->getEditIDs( ids );
}


bool ObjectEditor::addEditID( const EM::PosID& ) { return false; }


bool ObjectEditor::removeEditID( const EM::PosID& ) { return false; }


Coord3 ObjectEditor::getPosition( const EM::PosID& pid ) const
{ return emobject_->getPos( pid ); }


bool ObjectEditor::setPosition( const EM::PosID& pid,  const Coord3& np )
{
    const bool addtoundo = changedpids_.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids_ += pid;

    return emobject_->setPos( pid, np, addtoundo );
}

#define mMayFunction( func ) \
bool ObjectEditor::func( const EM::PosID& pid ) const \
{ \
    const Geometry::ElementEditor* ge = getEditor(); \
    if ( !ge ) return false; \
 \
    return ge->func( pid ); \
}


#define mGetFunction( func ) \
Coord3 ObjectEditor::func( const EM::PosID& pid ) const\
{\
    const Geometry::ElementEditor* ge = getEditor();\
    if ( !ge ) return Coord3::udf();\
\
    return ge->func( pid );\
}


mMayFunction( mayTranslate1D )
mGetFunction( translation1DDirection )

mMayFunction( mayTranslate2D )
mGetFunction( translation2DNormal )

mMayFunction( mayTranslate3D )

mMayFunction( maySetNormal )
mGetFunction( getNormal )

mMayFunction( maySetDirection )
mGetFunction( getDirectionPlaneNormal )
mGetFunction( getDirection )


Geometry::ElementEditor* ObjectEditor::getEditor()
{
    if ( geeditor_ ) return geeditor_;

    geeditor_ = createEditor();
    if ( geeditor_ )
    {
	geeditor_->editpositionchange.notify(
		mCB(this,ObjectEditor,editPosChangeTrigger));
    }

    return geeditor_;
}


const Geometry::ElementEditor* ObjectEditor::getEditor() const
{ return const_cast<ObjectEditor*>(this)->getEditor(); }



void ObjectEditor::editPosChangeTrigger(CallBacker*)
{ editpositionchange.trigger(); }


void ObjectEditor::getAlongMovingNodes( const EM::PosID&,
					TypeSet<EM::PosID>& nodes,
					TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors ) factors->erase();
}

} // namespace MPE

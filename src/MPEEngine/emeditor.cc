/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "emeditor.h"

#include "undo.h"
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

mImplFactory1Param( ObjectEditor, EM::EMObject&, EditorFactory );


ObjectEditor::ObjectEditor( EM::EMObject& emobj )
    : emobject_(&emobj)
    , editpositionchange(this)
    , movingnode_( -1,-1,-1 )
    , snapafteredit_( true )
    , nrusers_(0)
{}


ObjectEditor::~ObjectEditor()
{
    CallBack::removeFromThreadCalls( this );
    deepErase( geeditors_ );
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

    if ( pid.objectID()!=emobject_->id() )
    {
	movingnode_ = EM::PosID(-1,-1,-1);
	return;
    }

    movingnode_ = pid;

    startpos_ = getPosition(movingnode_);
    if ( !startpos_.isDefined() )
    {
	pErrMsg( "Editnode is undefined");
	movingnode_ = EM::PosID(-1,-1,-1);
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
    if ( movingnode_.objectID()==-1 )
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

    EM::EMM().undo().setUserInteractionEnd(
	    EM::EMM().undo().currentEventID() );
}


bool ObjectEditor::canSnapAfterEdit(const EM::PosID& pid) const
{
    if ( pid.objectID()!=emobject_->id() ||
	 MPE::engine().getTrackerByObject(emobject_->id())==-1 )
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

    for ( int idx=0; idx<emobject_->nrSections(); idx++ )
    {
	const EM::SectionID sectionid =  emobject_->sectionID(idx);
	const Geometry::ElementEditor* ge = getEditor( sectionid );
	if ( !ge ) continue;

	TypeSet<GeomPosID> gepids;
	ge->getEditIDs( gepids );
	for ( int idy=0; idy<gepids.size(); idy++ )
	    ids += EM::PosID( emobject_->id(), sectionid, gepids[idy] );
    }
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
    const Geometry::ElementEditor* ge = getEditor( pid.sectionID() ); \
    if ( !ge ) return false; \
 \
    return ge->func( pid.subID() ); \
}


#define mGetFunction( func ) \
Coord3 ObjectEditor::func( const EM::PosID& pid ) const\
{\
    const Geometry::ElementEditor* ge = getEditor( pid.sectionID() );\
    if ( !ge ) return Coord3::udf();\
\
    return ge->func( pid.subID() );\
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


Geometry::ElementEditor* ObjectEditor::getEditor( const EM::SectionID& sid )
{
    const int idx = sections_.indexOf(sid);
    if ( idx!=-1 ) return geeditors_[idx];

    Geometry::ElementEditor* geeditor = createEditor( sid );
    if ( geeditor )
    {
	geeditors_ += geeditor;
	sections_ += sid;
	geeditor->editpositionchange.notify(
		mCB(this,ObjectEditor,editPosChangeTrigger));
    }

    return geeditor;
}


const Geometry::ElementEditor* ObjectEditor::getEditor(
	const EM::SectionID& sid ) const
{ return const_cast<ObjectEditor*>(this)->getEditor(sid); }



void ObjectEditor::editPosChangeTrigger(CallBacker*)
{ editpositionchange.trigger(); }


void ObjectEditor::emSectionChange(CallBacker* cb)
{
    mCBCapsuleUnpack(const EM::EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event!=EM::EMObjectCallbackData::SectionChange )
	return;

    const EM::SectionID sectionid = cbdata.pid0.sectionID();
    const int editoridx = sections_.indexOf(sectionid);

    const Geometry::Element* ge =
	const_cast<const EM::EMObject*>(emobject_.ptr())
	    ->sectionGeometry(sectionid);

    if ( !ge && editoridx!=-1 )
    {
	delete geeditors_.removeSingle(editoridx);
	editpositionchange.trigger();
    }
    else if ( ge && editoridx==-1 )
    {
	Geometry::ElementEditor* geeditor = createEditor( sectionid );
	if ( geeditor )
	{
	    geeditors_ += geeditor;
	    sections_ += sectionid;
	    geeditor->editpositionchange.notify(
		    mCB(this,ObjectEditor,editPosChangeTrigger));
	}
    }
}


void ObjectEditor::getAlongMovingNodes( const EM::PosID&,
					TypeSet<EM::PosID>& nodes,
					TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors ) factors->erase();
}

} // namespace MPE

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emeditor.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emtracker.h"
#include "geeditor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"


MPE::ObjectEditor::ObjectEditor( const EM::EMObject& emobj )
    : editpositionchange(this)
{
    emobject_ = const_cast<EM::EMObject*>( &emobj );
}


MPE::ObjectEditor::~ObjectEditor()
{
    detachAllNotifiers();
    deepErase( geeditors );
}


ConstRefMan<EM::EMObject> MPE::ObjectEditor::emObject() const
{
    return emobject_.get();
}


RefMan<EM::EMObject> MPE::ObjectEditor::emObject()
{
    return emobject_.get();
}


BufferString MPE::ObjectEditor::objectName() const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    return emobject ? emobject->name() : BufferString::empty();
}


EM::ObjectID MPE::ObjectEditor::objectID() const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    return emobject ? emobject->id() : EM::ObjectID::udf();
}


static bool nodecloningenabled = false;
static int nodeclonecountdown = -1;

void MPE::ObjectEditor::enableNodeCloning( bool yn )
{
    nodecloningenabled = yn;
}


void MPE::ObjectEditor::startEdit( const EM::PosID& pid )
{
    changedpids.erase();

    alongmovingnodes.erase();
    alongmovingnodesfactors.erase();
    alongmovingnodesstart.erase();
    snapafterthisedit = false;

    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject || pid.objectID() != emobject->id() )
    {
	movingnode.setUdf();
	return;
    }

    movingnode = pid;

    startpos = getPosition(movingnode);
    if ( !startpos.isDefined() )
    {
	pErrMsg( "Editnode is undefined");
	movingnode.setUdf();
	return;
    }

    getAlongMovingNodes( pid, alongmovingnodes, &alongmovingnodesfactors );
    snapafterthisedit = snapafteredit && canSnapAfterEdit(pid);

    alongmovingnodesstart.erase();
    for ( int idx=0; idx<alongmovingnodes.size(); idx++ )
	alongmovingnodesstart += emobject->getPos(alongmovingnodes[idx]);

    nodeclonecountdown = nodecloningenabled ? 3 : -1;
}


bool MPE::ObjectEditor::setPosition(const Coord3& np)
{
    if ( !movingnode.isValid() )
    {
	pErrMsg("Moving unknown node");
	return false;
    }

    if ( !np.isDefined() )
    {
	pErrMsg("You cannot set the editnode to undefined");
	return false;
    }

    const Coord3 diff = np-startpos;
    if ( !setPosition( movingnode, np ) )
	return false;

    for ( int idx=0; idx<alongmovingnodes.size(); idx++ )
    {
	const Coord3 newpos = alongmovingnodesstart[idx] +
		    alongmovingnodesfactors[idx]*diff;
	if ( !setPosition( alongmovingnodes[idx], newpos ) )
	    return false;
    }

    nodeclonecountdown--;
    if ( !nodeclonecountdown )
	mMainThreadCall( ObjectEditor::cloneMovingNode );

    return true;
}


void MPE::ObjectEditor::finishEdit()
{
    if ( changedpids.isEmpty() )
	return;

    if ( snapafterthisedit )
    {
//	const int trackeridx = MPE::engine().getTrackerByObject(emobject.id());
//	EMTracker* tracker = MPE::engine().getTracker(trackeridx);
//	tracker->snapPositions(alongmovingnodes);
    }

    RefMan<EM::EMObject> emobject = emObject();
    if ( emobject )
	EM::EMM().undo( emobject->id()).setUserInteractionEnd(
		EM::EMM().undo(emobject->id()).currentEventID() );
}


bool MPE::ObjectEditor::canSnapAfterEdit( const EM::PosID& pid ) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject || pid.objectID()!=emobject->id() ||
	 !engine().hasTracker(emobject->id()) )
	return false;

    const TrcKeyZSampling& trackvolume = MPE::engine().activeVolume();

    TypeSet<EM::PosID> nodes;
    getAlongMovingNodes( pid, nodes, 0 );
    for ( int idx=0; idx<nodes.size(); idx++ )
    {
	const Coord3 pos = emobject->getPos(nodes[idx]);
	const BinID bid = SI().transform(pos);
	if ( !trackvolume.hsamp_.includes(bid) )
	    return false;

	if ( !trackvolume.zsamp_.includes(pos.z_,false) )
	    return false;
    }

    return true;
}


bool MPE::ObjectEditor::getSnapAfterEdit() const { return snapafteredit; }


void MPE::ObjectEditor::setSnapAfterEdit(bool yn) { snapafteredit=yn; }


void MPE::ObjectEditor::setEditIDs( const TypeSet<EM::PosID>* /* ids */ )
{
}


void MPE::ObjectEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    ids.erase();
    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const Geometry::ElementEditor* ge = getEditor();
	if ( !ge )
	    continue;

	TypeSet<GeomPosID> gepids;
	ge->getEditIDs( gepids );
	for ( int idy=0; idy<gepids.size(); idy++ )
	    ids += EM::PosID( emobject->id(), gepids[idy] );
    }
}


bool MPE::ObjectEditor::addEditID( const EM::PosID& )
{
    return false;
}


bool MPE::ObjectEditor::removeEditID( const EM::PosID& )
{
    return false;
}


Coord3 MPE::ObjectEditor::getPosition( const EM::PosID& pid ) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    return emobject ? emobject->getPos( pid ) : Coord3::udf();
}


bool MPE::ObjectEditor::setPosition( const EM::PosID& pid,  const Coord3& np )
{
    RefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return false;

    const bool addtoundo = changedpids.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids += pid;

    return emobject->setPos( pid, np, addtoundo );
}

#define mMayFunction( func ) \
bool MPE::ObjectEditor::func( const EM::PosID& pid ) const \
{ \
    const Geometry::ElementEditor* ge = getEditor(); \
    if ( !ge ) return false; \
 \
    return ge->func( pid.subID() ); \
}


#define mGetFunction( func ) \
Coord3 MPE::ObjectEditor::func( const EM::PosID& pid ) const\
{\
    const Geometry::ElementEditor* ge = getEditor();\
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


const Geometry::ElementEditor* MPE::ObjectEditor::getEditor() const
{
    return getNonConst(*this).getEditor();
}


Geometry::ElementEditor* MPE::ObjectEditor::getEditor()
{
    if ( !geeditors.isEmpty() )
	return geeditors.first();

    Geometry::ElementEditor* geeditor = createEditor();
    if ( geeditor )
    {
	geeditors += geeditor;
	mAttachCB( geeditor->editpositionchange,
		   ObjectEditor::editPosChangeTrigger );
    }

    return geeditor;
}


void MPE::ObjectEditor::editPosChangeTrigger( CallBacker* )
{
    editpositionchange.trigger();
}


void MPE::ObjectEditor::getAlongMovingNodes( const EM::PosID&,
					     TypeSet<EM::PosID>& nodes,
					     TypeSet<float>* factors ) const
{
    nodes.erase();
    if ( factors )
	factors->erase();
}

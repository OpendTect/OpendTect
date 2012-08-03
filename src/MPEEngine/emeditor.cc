/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: emeditor.cc,v 1.30 2012-08-03 06:38:39 cvsaneesh Exp $";

#include "emeditor.h"

#include "undo.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfaceedgeline.h"
#include "emtracker.h"
#include "geeditor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "rcollinebuilder.h"

namespace MPE 
{

mImplFactory1Param( ObjectEditor, EM::EMObject&, EditorFactory );


ObjectEditor::ObjectEditor( EM::EMObject& emobj_ )
    : emobject( emobj_ )
    , editpositionchange( this )
    , movingnode( -1,-1,-1 )
    , snapafteredit( true )
    , interactionline( 0 )
{
    emobject.ref();
}


ObjectEditor::~ObjectEditor()
{
    delete interactionline;
    emobject.unRef();
    deepErase( geeditors );
    sections.erase();
}


static bool nodecloningenabled = false;
static int nodeclonecountdown = -1;

void ObjectEditor::enableNodeCloning( bool yn )
{ nodecloningenabled = yn; } 


void ObjectEditor::startEdit(const EM::PosID& pid)
{
    changedpids.erase();

    alongmovingnodes.erase();
    alongmovingnodesfactors.erase();
    alongmovingnodesstart.erase();
    snapafterthisedit = false;

    if ( pid.objectID()!=emobject.id() )
    {
	movingnode = EM::PosID(-1,-1,-1);
	return;
    }

    movingnode = pid;

    startpos = getPosition(movingnode);
    if ( !startpos.isDefined() )
    {
	pErrMsg( "Editnode is undefined");
	movingnode = EM::PosID(-1,-1,-1);
	return;
    }

    getAlongMovingNodes( pid, alongmovingnodes, &alongmovingnodesfactors );
    snapafterthisedit = snapafteredit && canSnapAfterEdit(pid);

    alongmovingnodesstart.erase();
    for ( int idx=0; idx<alongmovingnodes.size(); idx++ )
	alongmovingnodesstart += emobject.getPos(alongmovingnodes[idx]);

    nodeclonecountdown = nodecloningenabled ? 3 : -1;
}


bool ObjectEditor::setPosition(const Coord3& np)
{
    if ( movingnode.objectID()==-1 )
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
	cloneMovingNode();

    return true;
}


void ObjectEditor::finishEdit()
{
    if ( changedpids.isEmpty() )
	return;

    if ( snapafterthisedit )
    {
	const int trackeridx = MPE::engine().getTrackerByObject(emobject.id());
	EMTracker* tracker = MPE::engine().getTracker(trackeridx);
	tracker->snapPositions(alongmovingnodes);
    }

    EM::EMM().undo().setUserInteractionEnd(
	    EM::EMM().undo().currentEventID() );
}


bool ObjectEditor::canSnapAfterEdit(const EM::PosID& pid) const
{
    if ( pid.objectID()!=emobject.id() ||
	 MPE::engine().getTrackerByObject(emobject.id())==-1 )
	return false;

    const CubeSampling& trackvolume = MPE::engine().activeVolume();

    TypeSet<EM::PosID> nodes;
    getAlongMovingNodes( pid, nodes, 0 );
    for ( int idx=0; idx<nodes.size(); idx++ )
    {
	const Coord3 pos = emobject.getPos(nodes[idx]);
	const BinID bid = SI().transform(pos);

	if ( !trackvolume.hrg.includes( bid ) )
	    return false;
	if ( !trackvolume.zrg.includes( pos.z,false ) )
	    return false;
    }

    return true;
}


bool ObjectEditor::getSnapAfterEdit() const { return snapafteredit; }


void ObjectEditor::setSnapAfterEdit(bool yn) { snapafteredit=yn; }


void ObjectEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();

    for ( int idx=0; idx<emobject.nrSections(); idx++ )
    {
	const EM::SectionID sectionid =  emobject.sectionID(idx);
	const Geometry::ElementEditor* ge = getEditor( sectionid );
	if ( !ge ) continue;

	TypeSet<GeomPosID> gepids;
	ge->getEditIDs( gepids );
	for ( int idy=0; idy<gepids.size(); idy++ )
	    ids += EM::PosID( emobject.id(), sectionid, gepids[idy] );
    }
}


bool ObjectEditor::addEditID( const EM::PosID& ) { return false; }


bool ObjectEditor::removeEditID( const EM::PosID& ) { return false; }


Coord3 ObjectEditor::getPosition( const EM::PosID& pid ) const
{ return emobject.getPos( pid ); }


bool ObjectEditor::setPosition( const EM::PosID& pid,  const Coord3& np )
{
    const bool addtoundo = changedpids.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids += pid;

    return emobject.setPos( pid, np, addtoundo );
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



mMayFunction( mayTranslate1D );
mGetFunction( translation1DDirection );


mMayFunction( mayTranslate2D );
mGetFunction( translation2DNormal );


mMayFunction( mayTranslate3D );


mMayFunction( maySetNormal );
mGetFunction( getNormal );


mMayFunction( maySetDirection );
mGetFunction( getDirectionPlaneNormal );
mGetFunction( getDirection );


void ObjectEditor::restartInteractionLine(const EM::PosID& pid)
{
    mDynamicCastGet( EM::Horizon3D*, emsurface, &emobject );
    const EM::SectionID sid = pid.sectionID();
    if ( !emsurface )
    {
	if ( interactionline )
	{
	    delete interactionline;
	    interactionline = 0;
	}
    }
    else if ( !interactionline )
    {
	EM::EdgeLine* el = new EM::EdgeLine( *emsurface, sid );
	interactionline = new EM::EdgeLineSet( *emsurface, sid );
	interactionline->addLine( el );
	el->setRemoveZeroSegments(false);
    }
    else
    {
	if ( interactionline->getLine(0)->nrSegments() )
	    interactionline->getLine(0)->getSegment(0)->removeAll();

	if ( sid!=interactionline->getSection() )
	    interactionline->setSection(sid);
    }

    if ( interactionline )
    {
	if ( !interactionline->getLine(0)->nrSegments() ||
	     !interactionline->getLine(0)->getSegment(0)->size() )
	{
	    //Start a new line
	    const RowCol activenoderc = pid.getRowCol();
	    EM::EdgeLine* edgeline = interactionline->getLine(0);
	    if ( !edgeline->nrSegments() )
	    {
		EM::EdgeLineSegment* elsegment=
		    new EM::EdgeLineSegment( *emsurface, sid );
		elsegment->insert( 0, activenoderc );
		edgeline->insertSegment(elsegment, 0, false);
	    }
	    else
	    {
		edgeline->getSegment(0)->insert( 0, activenoderc );
		const int sz = edgeline->getSegment(0)->size();
		if ( sz>1 )
		    edgeline->getSegment(0)->remove(1,sz-1);
	    }
	}
    }
}


bool ObjectEditor::closeInteractionLine( bool doit )
{
    if ( !interactionline || 
	 !interactionline->nrLines() ||
	 !interactionline->getLine(0)->nrSegments() ||
	 !interactionline->getLine(0)->getSegment(0)->size())
	return false;

    const RowCol rc(  (*interactionline->getLine(0)->getSegment(0))[0] );

    const EM::PosID pid( interactionline->getHorizon().id(),
	    		 interactionline->getSection(),
			 rc.toInt64() );

    return interactionLineInteraction( pid, doit );
}


bool ObjectEditor::interactionLineInteraction( const EM::PosID& pid,
       					       bool doit )
{
    if ( !interactionline || interactionline->getSection()!=pid.sectionID() ||
	 !interactionline->nrLines() ||
	 !interactionline->getLine(0)->nrSegments() ||
	 !interactionline->getLine(0)->getSegment(0)->size())
	return false;

    EM::Horizon3D& emsurface = interactionline->getHorizon();
    const EM::SectionID sid = interactionline->getSection();
    const RowCol rc = pid.getRowCol();

    EM::EdgeLineSegment& els = *interactionline->getLine(0)->getSegment(0);

    if ( els.indexOf(rc)!=-1 )
    {
	const int idx = els.indexOf(rc);
	if ( idx && idx<els.size()-1 )
	{
	    if ( doit )
		els.remove( idx+1, els.size()-1 );
	    return true;
	}
    }

    const RowCol lastrc = els.last();
    TypeSet <RowCol> line;
    makeLine( lastrc, rc, emsurface.geometry().step(), line );
    if ( rc==els[0] )
	line.remove(line.size()-1);

    for ( int idx=0; idx<line.size(); idx++ )
    {
	if ( !emsurface.isDefined(sid,line[idx].toInt64()) )
	    return false;
    }

    for ( int idx=1; idx<line.size(); idx++ )
    {
	if ( els.indexOf(line[idx])!=-1 )
	    return false;

	//Check that new line is not crossing the old one in a
	//diagonal

	const RowCol dir = line[idx]-line[idx-1];
	if ( !dir.row || !dir.col )
	    continue;

	const RowCol rownode = line[idx]-RowCol(dir.row,0);
	const RowCol colnode = line[idx]-RowCol(0, dir.col);
	const int rowindex = els.indexOf(rownode);
	const int colindex = els.indexOf(colnode);

	if ( rowindex==colindex-1 || rowindex==colindex+1 )
	    return false;
    }

    line.remove(0);
    if ( doit )
	els.insert( els.size(), line );
    return true;
}



	const EM::EdgeLineSet* ObjectEditor::getInteractionLine() const
{ return interactionline; }


Geometry::ElementEditor* ObjectEditor::getEditor( const EM::SectionID& sid )
{
    const int idx = sections.indexOf(sid);
    if ( idx!=-1 ) return geeditors[idx];

    Geometry::ElementEditor* geeditor = createEditor( sid );
    if ( geeditor )
    {
	geeditors += geeditor;
	sections += sid;
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
    const int editoridx = sections.indexOf(sectionid);

    const Geometry::Element* ge =
	const_cast<const EM::EMObject*>(&emobject)->sectionGeometry( sectionid );

    if ( !ge && editoridx!=-1 )
    {
	delete geeditors[editoridx];
	geeditors.remove(editoridx);
	editpositionchange.trigger();
    }
    else if ( ge && editoridx==-1 )
    {
	Geometry::ElementEditor* geeditor = createEditor( sectionid );
	if ( geeditor )
	{
	    geeditors += geeditor;
	    sections += sectionid;
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


}; //Namespace

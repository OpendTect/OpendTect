/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2004
 RCS:           $Id: uimpepartserv.cc,v 1.3 2005-01-13 09:51:18 kristofer Exp $
________________________________________________________________________

-*/

#include "uimpepartserv.h"

//#include "consistencychecker.h"
//#include "emhistory.h"
//#include "emhorizon.h"
//#include "emsurfacerelations.h"
//#include "errh.h"
//#include "executor.h"
//#include "tracker.h"
#include "geomelement.h"
#include "mpeengine.h"
#include "emtracker.h"
//#include "surfaceoptimizer.h"
//#include "attribdescset.h"
//#include "attribslice.h"
//#include "attribsel.h"
//#include "survinfo.h"
//#include "multiid.h"
//#include "trigonometry.h"
//#include "emsurfaceedgelineimpl.h"

//#include "uitrackingwizard.h"
//#include "uitrackingsetupdlg.h"
#include "uimsg.h"
#include "uicursor.h"
//#include "uidialog.h"
//#include "uimenu.h"
//#include "uisurfacerelationdlg.h"
//
//#include "emsurfaceedgeline.h"
#include "emmanager.h"
#include "emobject.h"
//#include "emsurface.h"
//#include "emsurfacegeometry.h"
//
//#include <math.h>


const int uiMPEPartServer::evGetAttribData	= 0;
const int uiMPEPartServer::evStartSeedPick	= 1;
const int uiMPEPartServer::evEndSeedPick	= 2;
const int uiMPEPartServer::evAddTreeObject	= 3;
//const int uiMPEPartServer::evCheckStickSet	  = 4;
//const int uiMPEPartServer::evFinishInit	  = 5;
//const int uiMPEPartServer::evShowManager	  = 6;
//const int uiMPEPartServer::evGetData	  = 7;
//const int uiMPEPartServer::evInitVisStuff	  = 8;


uiMPEPartServer::uiMPEPartServer( uiApplService& a, const AttribDescSet* ads )
    : uiApplPartServer(a)
    , attrset( ads )
{
    MPE::initStandardClasses();
}


uiMPEPartServer::~uiMPEPartServer()
{ }


void uiMPEPartServer::setCurrentAttribDescSet( const AttribDescSet* ads )
{ attrset = ads; }


int uiMPEPartServer::getTrackerID( const MultiID& mid ) const
{
    for ( int idx=0; idx<MPE::engine().highestTrackerID(); idx++ )
    {
	if ( MPE::engine().getTracker(idx) )
	{
	    EM::ObjectID objid = MPE::engine().getTracker(idx)->objectID();
	    if ( objid==-1 )
		continue;

	    EM::EMObject* emobj = EM::EMM().getObject(objid);
	    if ( emobj && emobj->multiID()==mid )
		return idx;
	}
    }

    return -1;
}



int uiMPEPartServer::getTrackerID(const char* name) const
{
    return MPE::engine().getTrackerByObject(name);
}


void uiMPEPartServer::getTrackerTypes(BufferStringSet& res) const
{ MPE::engine().getAvaliableTrackerTypes(res); }


int uiMPEPartServer::addTracker(const MultiID&)
{
   /* 
    curemid = mid;
    cursectionid = section;
    setObjectType( mid );
    startWizard( 2 );
    wizard->setTracker( addTracker(mid) );

    csfromsticks.zrg.start = csfromsticks.zrg.stop = pickedpos.z;
    HorSampling& hrg = csfromsticks.hrg;
    const BinID bid = SI().transform(pickedpos);
    hrg.start.inl = bid.inl - 10 * hrg.step.inl;
    hrg.stop.inl = bid.inl + 10 * hrg.step.inl;
    hrg.start.crl = bid.crl - 10 * hrg.step.crl;
    hrg.stop.crl = bid.crl + 10 * hrg.step.crl;
    */
    pErrMsg("Not impl");
    return -1;
}


int uiMPEPartServer::addTracker( const char* trackertype )
{
    activetrackerid = MPE::engine().addTracker("New object", trackertype );
    if ( activetrackerid==-1 )
	uiMSG().error( MPE::engine().errMsg() );
    else
    {
	//Create Editor
	const EM::ObjectID objid =
	    MPE::engine().getTracker(activetrackerid)->objectID();
	if ( !MPE::engine().getEditor(objid,false) )
	    MPE::engine().getEditor(objid,true);

	if ( !sendEvent( evAddTreeObject ) )
	{
	    pErrMsg("Could not add treeitem");
	    MPE::engine().removeTracker( activetrackerid );
	    activetrackerid = -1;
	    //TODO? Remove new object?
	    //TODO? Remove new editor?
	}
    }

    return activetrackerid;
}


MultiID uiMPEPartServer::getTrackerMultiID( int trackerid ) const
{
    const MPE::EMTracker* emt = MPE::engine().getTracker(trackerid);
    if ( emt )
    {
	const EM::EMObject* emo = EM::EMM().getObject(emt->objectID());
	if ( emo ) return emo->multiID();
    }

   return MultiID(-1);
}


bool uiMPEPartServer::canAddSeed( int trackerid ) const
{
    pErrMsg("Not impl");
    return false;
}


void uiMPEPartServer::addSeed( int trackerid )
{
    /*
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    mDynamicCastGet(EM::Surface*,surface,EM::EMM().getObject(objid))
    if ( !surface ) return false;

    curemid = mid;
    cursectionid = surface->geometry.addSection( "", true );
    setObjectType( mid );
    startWizard( 1 );
    wizard->setSurfaceColor( surface->preferredColor() );

    int trackidx = getTrackerID( curemid );
    if ( trackidx < 0 )
	trackidx = trackman.addTracker( curemid );

    wizard->setTracker( trackman.getTracker(trackidx) );

    return true;
    */
}


bool uiMPEPartServer::isTrackingEnabled( int trackerid ) const
{ return MPE::engine().getTracker(trackerid)->isEnabled(); }


void uiMPEPartServer::enableTracking( int trackerid, bool yn )
{ return MPE::engine().getTracker(trackerid)->enable(yn); }


int uiMPEPartServer::activeTrackerID() const
{ return activetrackerid; }


const AttribSelSpec* uiMPEPartServer::getAttribSelSpec() const
{ return eventattrselspec; }


/*
bool uiMPEPartServer::startWizard( int startidx )
{
    csfromsticks.init( false );
    csfromsticks.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
    csfromsticks.zrg.step = SI().zRange(false).step;

    delete wizard;
    wizard = new Tracking::Wizard( appserv().parent(), this );
    wizard->startAt( startidx );
    wizard->setType( curtype == uiMPEPartServer::Hor );
    wizard->go();
    return true;
}
*/


void uiMPEPartServer::showSetupDlg( int trackerid )
{
    /*
    const int tid = getTrackerID( mid );
    Tracking::Tracker* tracker = tid < 0 ? 0 : trackman.getTracker( tid );
    if ( !tracker ) 
    { uiMSG().error( "Cannot find setup for this surface" ); return; }

    const EM::ObjectID id = EM::EMM().multiID2ObjectID( mid );
    const bool ishor = EM::EMM().type( id ) == EM::EMManager::Hor;

    uiDialog dlg( appserv().parent(), uiDialog::Setup("Tracking Setup") );
    dlg.setOkText( "Dismiss" );
    dlg.setCancelText("");
    dlg.setHelpID( "108.0.1" );
    Tracking::uiSetupGroup* grp = new Tracking::uiSetupGroup( &dlg, attrset );
    grp->setType( ishor );
    grp->setTracker( tracker );
    if ( dlg.go() )
    {
	getData();
	sendEvent( evShowManager );
    }
    */
}


void uiMPEPartServer::showRelationsDlg( int trackerid )
{}


void uiMPEPartServer::setAttribData( const AttribSelSpec& spec,
				     AttribSliceSet* slcset )
{
    MPE::engine().setAttribData( spec, slcset );
}


void uiMPEPartServer::loadAttribData()
{
    uiCursorChanger changer( uiCursor::Wait );

    ObjectSet<const AttribSelSpec> attribselspecs;
    MPE::engine().getNeededAttribs(attribselspecs);
    for ( int idx=0; idx<attribselspecs.size(); idx++ )
    {
	eventattrselspec = attribselspecs[idx];
	sendEvent( evGetAttribData );
    }
}


const AttribSliceSet*
uiMPEPartServer::getAttribCache( const AttribSelSpec& spec ) const
{ return MPE::engine().getAttribCache( spec ); }


CubeSampling uiMPEPartServer::getAttribCube(  const AttribSelSpec& spec ) const
{ return MPE::engine().getAttribCube(spec); }

/*
void uiMPEPartServer::calcInterpreterCube( const TypeSet<Coord3>& stick )
{
    for ( int idx=0; idx<stick.size(); idx++ )
    {
	Coord3& pos = stick[idx];
	BinID bid = SI().transform( pos );
	if ( csfromsticks.isEmpty() )
	{
	    csfromsticks.hrg.start.inl = csfromsticks.hrg.stop.inl = bid.inl;
	    csfromsticks.hrg.start.crl = csfromsticks.hrg.stop.crl = bid.crl;
	    csfromsticks.zrg.start = csfromsticks.zrg.stop = pos.z;
	}
	else
	{
	    csfromsticks.hrg.include( bid );
	    csfromsticks.zrg.include( pos.z );
	}
    }
}

*/

/*
void uiMPEPartServer::finishAll()
{
    if ( !trackman.nrTrackers() ) return;

    addInterpreter();

    const CubeSampling& oldcs = trackman.getWorkCube();
    CubeSampling newcube = oldcs.isEmpty() ? csfromsticks : oldcs;
    if ( !csfromsticks.isEmpty() )
    {
	newcube.hrg.include( csfromsticks.hrg.start );
	newcube.hrg.include( csfromsticks.hrg.stop );
	newcube.zrg.include( csfromsticks.zrg.start );
	newcube.zrg.include( csfromsticks.zrg.stop );
    }

    if ( !newcube.hrg.start.inl && !newcube.hrg.start.crl &&
	    !newcube.hrg.stop.inl && !newcube.hrg.stop.crl )
    {
	CubeSampling workcube;
	const int inlwidth = workcube.hrg.stop.inl-workcube.hrg.start.inl;
	const int crlwidth = workcube.hrg.stop.crl-workcube.hrg.start.crl;
	const float zwidth = workcube.zrg.width();

	workcube.hrg.start.inl += inlwidth/3;
	workcube.hrg.stop.inl -= inlwidth/3;
	workcube.hrg.start.crl += crlwidth/3;
	workcube.hrg.stop.crl -= crlwidth/3;
	workcube.zrg.start += zwidth/3;
	workcube.zrg.stop -= zwidth/3;
	workcube.snapToSurvey(true);
	newcube = workcube;
    }

    if ( newcube.nrInl()<2 )
    {
	newcube.hrg.start.inl -= 100;
	newcube.hrg.stop.inl += 100;
    }

    if ( newcube.nrCrl()<2 )
    {
	newcube.hrg.start.crl -= 100;
	newcube.hrg.stop.crl += 100;
    }

    if ( oldcs.isEmpty() )
	newcube.zrg.widen( 0.1 );

    newcube.snapToSurvey();
    newcube.limitTo( SI().sampling(true) );
    setWorkCube( newcube );

    sendEvent( evShowManager );
}


#define mTransferPositions( set ) \
for ( int idx=0; idx<set.size(); idx++ ) \
{ \
    const EM::PosID newposid( set[idx].objectID(), newsection, \
			      set[idx].subID() ); \
    cuttedsurface->setPos( newposid, \
			   cuttedsurface->getPos(set[idx]), true); \
    for ( int idy=0; idy<cuttedsurface->nrPosAttribs(); idy++ ) \
    { \
	const int attrib = cuttedsurface->posAttrib(idy); \
	const bool ison = \
	    cuttedsurface->isPosAttrib( set[idx], attrib ); \
 \
	cuttedsurface->setPosAttrib( newposid, attrib, ison ); \
    } \
} 

bool uiMPEPartServer::terminateSurfaceBy( const EM::ObjectID& cuttedobj,
		 const EM::SectionID& cuttedsection,
		 const EM::ObjectID& cuttingobj,
		 const EM::SectionID& cuttingsection, bool positiveside )
{
    mDynamicCastGet(EM::Surface*,cuttedsurface,EM::EMM().getObject(cuttedobj))
    mDynamicCastGet(EM::Surface*,cuttingsurface,EM::EMM().getObject(cuttingobj))
    if ( !cuttedsurface || !cuttingsurface ||
	    !cuttedsurface->geometry.hasSection(cuttedsection) ||
	    !cuttingsurface->geometry.hasSection(cuttingsection) )
	return false;

    EM::SurfaceCutLine* cutline = 0;
    EM::EdgeLineSet& elset =
	*cuttedsurface->edgelinesets.getEdgeLineSet(cuttedsection,true);
    if ( !&elset ) return false;
    int mainline =  elset.getMainLine();
    if ( mainline==-1 )
    {
	for ( int idx=0; idx<elset.nrLines(); idx++ )
	    elset.getLine( idx )->reTrackLine();
	mainline =  elset.getMainLine();
    }
    EM::EdgeLine& el = *elset.getLine( mainline );
    if ( !&el ) return false;

    const int relation = cuttedsurface->relations.setRelation(
		      cuttedsection, cuttingobj, cuttingsection, positiveside );

    cutline = EM::SurfaceCutLine::createCutFromEdges(*cuttedsurface,
		cuttedsection, relation, trackman.time2DepthFunc() );

    if ( !cutline || cutline->size()<2 )
    {
	delete cutline;
	return false;
    }

    ObjectSet<EM::EdgeLineSegment> newsections;
    newsections += cutline;

    if ( el.getSegment(cutline->first())==-1 ||
         el.getSegment(cutline->last())==-1 ) 
    {
	const bool needsstart = el.getSegment(cutline->first())==-1;

	const RowCol growthdir = needsstart
		    ? (cutline->first()-cutline->last()).getDirection()
		    : (cutline->last()-cutline->first()).getDirection();

	const RowCol& step = cuttedsurface->geometry.step();
	const RowCol helplinestart = needsstart?cutline->first():cutline->last();
	const RowCol firstrc = helplinestart + growthdir*step;

	RowCol currc = firstrc;
	while ( el.getSegment(currc)==-1 )
	    currc += growthdir*step;

	EM::EdgeLineSegment* helpline = 
	    new EM::EdgeLineSegment(*cuttedsurface, cuttedsection );

	if ( needsstart )
	    helpline->makeLine( currc, firstrc );
	else
	    helpline->makeLine( firstrc, currc );

	if ( needsstart ) newsections.insertAt(helpline,0);
	else newsections += helpline;
    }

    TypeSet<RowCol> linercs;
    for ( int idx=0; idx<newsections.size(); idx++ )
	linercs.append(newsections[idx]->getNodes() );
    
    el.insertSegments( newsections, -1, true );

    elset.removeAllNodesOutsideLines();
    reTrackToCut( cuttedsurface, cuttedsection, relation, linercs );

    return true;
}

#define mErrRetCutSurfaceBy \
{ \
    while ( EM::EMM().history().canUnDo() &&\
	    EM::EMM().history().currentEventNr()!=initialhistnr )\
    {\
	bool res = EM::EMM().history().unDo(1);\
	if ( !res ) break;\
    }\
    \
    EM::EMM().history().setCurrentEventAsLast();\
    cutline1; \
    cutline2; \
    return false; \
}


EM::SectionID
uiMPEPartServer::cutSurfaceBy( const EM::ObjectID& cuttedobj,
				    const EM::SectionID& cuttedsection,
				    const EM::ObjectID& cuttingobj,
				    const EM::SectionID& cuttingsection )
{
    mDynamicCastGet(EM::Surface*,cuttedsurface,EM::EMM().getObject(cuttedobj))
    if ( !cuttedsurface || !cuttedsurface->geometry.hasSection(cuttedsection))
	return -1;

    mDynamicCastGet(EM::Surface*,cuttingsurface,EM::EMM().getObject(cuttingobj))
#ifdef __debug__
    if ( cuttedsurface->isChanged(-1) || cuttingsurface->isChanged(-1) )
    {
	pErrMsg("Saving surfaces before cut for debugging purposes."
		"This should be removed when cutting has stabalized.");
	PtrMan<Executor> cuttedsaver = cuttedsurface->isChanged(-1)
	    ? cuttedsurface->saver()
	    : 0;
	PtrMan<Executor> cuttingsaver = cuttedsurface->isChanged(-1)
	    ? cuttingsurface->saver() 
	    : 0;
	if ( cuttedsaver) cuttedsaver->execute();
	if ( cuttingsaver ) cuttingsaver->execute();
    }
#endif

    const int initialhistnr = EM::EMM().history().currentEventNr();
    const EM::SectionID newsection =
	cuttedsurface->geometry.cloneSection(cuttedsection);
    if ( newsection==-1 )
	return -1;

    if (!terminateSurfaceBy( cuttedobj,cuttedsection,
			     cuttingobj,cuttingsection,true)||
	!terminateSurfaceBy( cuttedobj,newsection,
	    		     cuttingobj,cuttingsection,false))
    {
	while ( EM::EMM().history().canUnDo() &&
		EM::EMM().history().currentEventNr()!=initialhistnr )
	{
	    bool res = EM::EMM().history().unDo(1);
	    if ( !res ) break;
	}
	
	EM::EMM().history().setCurrentEventAsLast();
	return -1; 
    }

    return newsection;
}

*/
/*
void uiMPEPartServer::reTrackToCut( EM::Surface* cuttedsurface,
			     const EM::SectionID& cuttedsection,
			     int relation,
			     const TypeSet<RowCol>& cutline )
{
    if ( cutline.size()<2 ) return;

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    Interval<int> cutrowrange, cutcolrange;
    RowCol cutlineinwarddir(0,0);
    for ( int idx=0; idx<cutline.size(); idx++ )
    {
	if ( idx )
	{
	    cutrowrange.include( (cutline)[idx].row );
	    cutcolrange.include( (cutline)[idx].col );

	    const RowCol& backdir =
		((cutline)[idx-1]-(cutline)[idx]).getDirection();
	    const int backdiridx = dirs.indexOf(backdir);
	    const int inwarddir = (backdiridx-2+dirs.size())%dirs.size();
	    cutlineinwarddir += dirs[inwarddir];
	}
	else
	{
	    cutrowrange.start = cutrowrange.stop = (cutline)[idx].row;
	    cutcolrange.start = cutcolrange.stop = (cutline)[idx].col;
	}
    }

    const MathFunction<float>* t2d = trackman.time2DepthFunc();


    bool trackcols = cutcolrange.width() < cutrowrange.width();
    bool inc = trackcols ? cutlineinwarddir.col<0 : cutlineinwarddir.row<0;

    RowCol nearestctrlnode;
    bool res = getNearestCtrlNode( trackcols, inc, cuttedsurface,
	    			   cuttedsection, cutrowrange, cutcolrange,
				   nearestctrlnode );
    if ( !res )
    {
	trackcols = !trackcols;
	inc = trackcols ? cutlineinwarddir.col<0 : cutlineinwarddir.row<0;
	res = getNearestCtrlNode( trackcols, inc, cuttedsurface,
				  cuttedsection, cutrowrange, cutcolrange,
				  nearestctrlnode );
	if ( !res ) return;
    }


    //Remove old nodes (that eventually will be replaced)
    const RowCol& step = cuttedsurface->geometry.step();
    const Interval<int> removerowrange( trackcols
		? cutrowrange.start
		: nearestctrlnode.row+(inc?step.row:-step.row),
	    trackcols
		? cutrowrange.stop
	        : (inc ? cutrowrange.stop : cutrowrange.start ) );
    const Interval<int> removecolrange( !trackcols
		? cutcolrange.start
		: (inc?nearestctrlnode.col+step.col:cutcolrange.start),
	    !trackcols
		? cutcolrange.stop
	        : (inc ? cutcolrange.stop : nearestctrlnode.col-step.col) );
    for ( int row=removerowrange.start; removerowrange.includes(row);
	    row += step.row )
    {
	for ( int col=removecolrange.start; removecolrange.includes(col);
	    col += step.col )
	{
	    cuttedsurface->geometry.setPos(cuttedsection,RowCol(row,col),
		    Coord3::udf(), false, true);
	}
    }

    TypeSet<EM::PosID> sources;
    for ( int idx=trackcols ? cutrowrange.start : cutcolrange.start;
	  trackcols ? cutrowrange.includes( idx ) : cutcolrange.includes( idx );
	  idx += trackcols ? step.row : step.col )
    {
	const RowCol rc (trackcols?idx:nearestctrlnode.row,
		         trackcols?nearestctrlnode.col:idx );
	const EM::PosID pid( cuttedsurface->id(), cuttedsection,
			     EM::SurfaceGeometry::rowCol2SubID(rc) );

	sources += pid;
    }


    const RowCol dir( trackcols ? 0 : (inc?step.row:-step.row),
	    	      trackcols ? (inc?step.col:-step.col) : 0 );

    bool track = false;
    while ( sources.size() )
    {
	for ( int idx=0; idx<sources.size(); idx++ )
	{
	    const RowCol rc =
		EM::SurfaceGeometry::subID2RowCol(sources[idx].subID());

	    //if ( !cuttedsurface->geometry.isDefined(cuttedsection,rc) )
	    //{ sources.remove(idx--); continue; }
	}

	if ( !sources.size() )
	    break;

	if ( track )
	{
	    const BinIDValue bidval(EM::HorizonGeometry::getBinID(dir),0.0);
	    trackman.track(sources,bidval,true);
	    //TODO Generate new sources...
	}
	else
	{
	    TypeSet<EM::PosID> targets;
	    Tracking::ConsistencyChecker consistencychecker;
	    consistencychecker.setSurface( cuttedsurface );
	    consistencychecker.setTimeToDepthFunction(t2d);

	    for ( int idx=0; idx<sources.size(); idx++ )
	    {
		const RowCol targetrc =
		    EM::SurfaceGeometry::subID2RowCol(sources[idx].subID())+dir;
		 if ( !removerowrange.includes(targetrc.row) ||
		      !removecolrange.includes(targetrc.col))
		     continue;

		const EM::PosID target( sources[idx].objectID(),
		    cuttedsection, EM::SurfaceGeometry::rowCol2SubID(targetrc));

		const Coord3 newpos = cuttedsurface->isDefined(sources[idx]) ?
		    cuttedsurface->getPos(sources[idx]) :
		    cuttedsurface->geometry.getPos( cuttedsection,nearestctrlnode);
		cuttedsurface->geometry.setPos(cuttedsection,targetrc,newpos,
			  false, true );
		consistencychecker.addNodeToCheck(target);
		targets += target;
	    }

	    sources.erase();
	    sources.append( targets );
	}
    }

    Tracking::ConsistencyChecker consistencychecker;
    consistencychecker.setSurface( cuttedsurface );
    consistencychecker.setTimeToDepthFunction(t2d);
    while ( consistencychecker.nextStep() )
	;


    EM::EdgeLineSet& elset =
	*cuttedsurface->edgelinesets.getEdgeLineSet(cuttedsection,true);
    if ( !&elset ) return;
    EM::EdgeLine& el = *elset.getLine( elset.getMainLine() );
    if ( !&el ) return;

    EM::SurfaceCutLine* newcutline =
	EM::SurfaceCutLine::createCutFromEdges( *cuttedsurface,
					cuttedsection, relation, t2d );

    if ( !newcutline || newcutline->size()<2 )
    {
	delete newcutline;
	return;
    }

    ObjectSet<EM::EdgeLineSegment> newsections;
    newsections += newcutline;

    if ( el.getSegment(newcutline->first())==-1 ||
	 el.getSegment(newcutline->last())==-1 )
    {
	const bool needsstart = el.getSegment(newcutline->first())==-1;
	const RowCol helplinestart =
	    needsstart?newcutline->first():newcutline->last();
	const RowCol growthdir = needsstart
		? (newcutline->first()-newcutline->last()).getDirection()
		: (newcutline->last()-newcutline->first()).getDirection();
	const RowCol firstrc = helplinestart + growthdir*step;

	RowCol currc = firstrc;
	while ( el.getSegment(currc)==-1 )
	    currc += growthdir*step;

	EM::EdgeLineSegment* helpline =
	    new EM::EdgeLineSegment(*cuttedsurface, cuttedsection );
	if ( needsstart )
	    helpline->makeLine( currc, firstrc );
	else
	    helpline->makeLine( firstrc, currc );

	if ( needsstart ) newsections.insertAt(helpline,0);
	else newsections += helpline;
    }

    el.insertSegments( newsections, -1, true );
    elset.removeAllNodesOutsideLines();

}



bool uiMPEPartServer::handleTrackerMenu( const MultiID& mid,
					      const EM::SectionID& cuttedsect,
					      const Coord3& pickedpos )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID cuttedid = em.multiID2ObjectID( mid );
    mDynamicCastGet(const EM::Surface*,cuttedsurface,em.getObject(cuttedid))
    
    BufferString title( "Set relations for '" );
    title += em.name(cuttedid); title += "'";
    bool allowhorsel = false;
    bool allowfltsel = true;
    uiSurfaceRelationDlg dlg( appserv().parent(), title, cuttedsurface,
			      allowhorsel, allowfltsel );
    if ( !dlg.go() ) return false;

    TypeSet<EM::ObjectID> ids;
    dlg.getNewRelations( true, ids );
    for ( int cuttingsurfidx=0; cuttingsurfidx<ids.size(); cuttingsurfidx++ )
    {
	EM::ObjectID cuttingid = ids[cuttingsurfidx];
	mDynamicCastGet(const EM::Surface*,cuttingsurf,em.getObject(cuttingid));
	if ( !cuttingsurf ) continue;
	for ( int cuttingsectionidx=0;
		cuttingsectionidx<cuttingsurf->geometry.nrSections();
		cuttingsectionidx++ )
	{
	    const int nrcuttedsections = cuttedsurface->geometry.nrSections();
	    for ( int cuttedsurfacesectionidx=0;
		  cuttedsurfacesectionidx<nrcuttedsections;
		  cuttedsurfacesectionidx++ )
	    {
		EM::SectionID newsect = cutSurfaceBy( cuttedid,
		    cuttedsurface->geometry.sectionID(cuttedsurfacesectionidx),
		    cuttingid,
		    cuttingsurf->geometry.sectionID(cuttingsectionidx));

		if ( newsect!=-1 )
		    continue;

		const float dist2fault = cuttingsurf->geometry.
		    normalDistance( pickedpos, trackman.time2DepthFunc() );

		cuttedsurface->relations.setRelation(
		    cuttedsurface->geometry.sectionID(cuttedsurfacesectionidx),
		    cuttingid,
		    cuttingsurf->geometry.sectionID(cuttingsectionidx),
		    dist2fault<0, true );
	    }
	}
    }

    ids.erase();
    dlg.getNewRelations( false, ids );
    for ( int cuttingsurfidx=0; cuttingsurfidx<ids.size(); cuttingsurfidx++ )
    {
	EM::ObjectID cuttingid = ids[cuttingsurfidx];
	mDynamicCastGet(const EM::Surface*,cuttingsurf,em.getObject(cuttingid));
	if ( !cuttingsurf ) continue;
	for ( int cuttingsectionidx=0;
		cuttingsectionidx<cuttingsurf->geometry.nrSections();
		cuttingsectionidx++ )
	{
	    const int nrcuttedsections = cuttedsurface->geometry.nrSections();
	    for ( int cuttedsurfacesectionidx=0;
		  cuttedsurfacesectionidx<nrcuttedsections;
		  cuttedsurfacesectionidx++ )
	    {
		const float dist2fault = cuttingsurf->geometry.
		    normalDistance( pickedpos, trackman.time2DepthFunc() );

		terminateSurfaceBy( cuttedid,
		    cuttedsurface->geometry.sectionID(cuttedsurfacesectionidx),
		    cuttingid,
		    cuttingsurf->geometry.sectionID(cuttingsectionidx),
		    dist2fault<0);
	    }
	}
    }


    ids.erase();
    dlg.getRemovedRelations( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	EM::ObjectID objid = ids[idx];
	mDynamicCastGet(const EM::Surface*,surface,em.getObject(objid));
	if ( !surface ) continue;

	for ( int idy=0; idy<surface->geometry.nrSections(); idy++ )
	    cuttedsurface->relations.removeRelation( cuttedsect, objid,
		    			  surface->geometry.sectionID(idy) );
    }
    
    return true;
}


bool uiMPEPartServer::getNearestCtrlNode( bool trackcols, bool inc,
			const EM::Surface* cuttedsurface, 
			const EM::SectionID& cuttedsection,
			const Interval<int>& cutrowrange,
			const Interval<int>& cutcolrange,
			RowCol& nearestctrlnode )
{
    const TypeSet<EM::PosID>& ctrlnodes = *cuttedsurface->getPosAttribList(
					EM::EMObject::sPermanentControlNode );
    if ( !&ctrlnodes || !ctrlnodes.size() )
	return -1;

    int mindist;
    int minidx = -1;
    for ( int idx=0; idx<ctrlnodes.size(); idx++ )
    {
	EM::PosID& posid = ctrlnodes[idx];
	const RowCol controlrc =
	    cuttedsurface->geometry.subID2RowCol( posid.subID() );

	if ( posid.sectionID()!=cuttedsection || 
		!cuttedsurface->isDefined(posid) )
	    continue;

	if ( trackcols )
	{
	    if ( inc && controlrc.col>=cutcolrange.start )
		continue;
	    if ( !inc && controlrc.col<=cutcolrange.stop )
		continue;
	}
	else
	{
	    if ( inc && controlrc.row>=cutrowrange.start )
		continue;
	    if ( !inc && controlrc.row<=cutrowrange.stop )
		continue;
	}

	const int dist = trackcols
			    ? mMIN(abs(controlrc.col-cutcolrange.start),
				   abs(controlrc.col-cutcolrange.stop) )
			    : mMIN(abs(controlrc.row-cutrowrange.start),
				   abs(controlrc.row-cutrowrange.stop) );

	if ( minidx==-1 || dist<mindist )
	{
	    mindist = dist;
	    minidx = idx;
	}
    }

    if ( minidx < 0 ) return false;

    nearestctrlnode = 
	    cuttedsurface->geometry.subID2RowCol( ctrlnodes[minidx].subID() );
    return true;
    
}
*/


void uiMPEPartServer::setSeed( ObjectSet<Geometry::Element>& newseeds )
{
    deepErase( MPE::engine().interactionseeds );
    MPE::engine().interactionseeds = newseeds;
    newseeds.erase();
}


void uiMPEPartServer::fillPar( IOPar& par ) const
{
    MPE::engine().fillPar( par );
}


bool uiMPEPartServer::usePar( const IOPar& par )
{
    bool res = MPE::engine().usePar( par );
    /*
    if ( res )
    {
	if ( !sendEvent(evInitVisStuff) )
	    return false;
	res = getData();
	sendEvent( evShowManager );
    }
    */
    return res;
}

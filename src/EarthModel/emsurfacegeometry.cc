/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurfacegeometry.cc,v 1.12 2005-01-28 16:14:55 nanne Exp $";

#include "emsurfacegeometry.h"

#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfacerelations.h"
#include "emsurfaceauxdata.h"

#include "emsurfaceedgeline.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "geomgridsurface.h"
#include "geommeshsurface.h"
#include "mathfunc.h"
#include "ioman.h"
#include "ioobj.h"
#include "pca.h"
#include "toplist.h"
#include "survinfo.h"

namespace EM {


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


SurfaceGeometry::SurfaceGeometry( Surface& surf_ )
    : step_(SI().inlStep(),SI().crlStep())
    , loadedstep(SI().inlStep(),SI().crlStep())
    , rowinterval(0)
    , colinterval(0)
    , shift(0)
    , changed( 0 )
    , surface( surf_ )
{}


SurfaceGeometry::~SurfaceGeometry()
{
    removeAll();
}


void SurfaceGeometry::removeAll()
{
    while ( nrSections() ) removeSection(sectionID(0), false);

    delete rowinterval;
    delete colinterval;
    rowinterval = 0;
    colinterval = 0;
}


bool SurfaceGeometry::findClosestNodes( TopList<float,PosID>& toplist,
				    const Coord3& pos_,
				    const FloatMathFunction* t2dfunc) const
{
    const int nrsections = nrSections();
    for ( int section=0; section<nrsections; section++ )
	findClosestNodes( sectionID(section), toplist, pos_, t2dfunc );

    return toplist.size();
}


bool SurfaceGeometry::findClosestNodes( const SectionID& sectionid,
				    TopList<float,PosID>& toplist,
				    const Coord3& pos_,
				    const FloatMathFunction* t2dfunc ) const
{
    toplist.setTop(false);

    //TODO Make faster impl
    Coord3 origpos = pos_;
    if ( t2dfunc ) origpos.z = t2dfunc->getValue( pos_.z );

    StepInterval<int> rowrange; StepInterval<int> colrange;
    getRange( rowrange, true );
    if ( rowrange.width(false)<0 )
	return false;
    
    getRange( colrange, false );

    RowCol rc;
    for ( rc.row=rowrange.start;rc.row<=rowrange.stop;rc.row+=rowrange.step)
    {
	for ( rc.col=colrange.start; rc.col<=colrange.stop;
						    rc.col+=colrange.step )
	{
	    if ( isDefined(sectionid,rc) )
	    {
		Coord3 pos = getPos( sectionid, rc );
		if ( t2dfunc )
		    pos.z = t2dfunc->getValue( pos.z );

		double dist = pos.distance( origpos );
		toplist.addValue( dist,
			      PosID(surface.id(),sectionid,rowCol2SubID(rc)) );
	    
	    }
	}
    }

    return toplist.size();
}


bool SurfaceGeometry::findClosestMesh( PosID& res, const Coord3& timepos,
			           const FloatMathFunction* t2dfunc ) const
{
    TopList<float, PosID> closestnodes( 20, mUndefValue, false );
    if ( !findClosestNodes(closestnodes,timepos,t2dfunc) )
	return false;

    const Coord3 pos = t2dfunc ? Coord3( timepos, t2dfunc->getValue(timepos.z) )
			       : timepos;

    float mindist;
    bool isresset = false;
    const int nrnodes = closestnodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	PosID pid = closestnodes.getAssociatedValue(idx);
	Coord3 c00, c10, c01, c11;
	bool c00def, c10def, c01def, c11def;
	getMeshCoords( pid, c00, c10, c01, c11, 
		       c00def, c10def, c01def, c11def, t2dfunc );

	int nrvalidcoords = 0;
	float totaldist = 0;
	if ( c00def ) { nrvalidcoords++; totaldist+=c00.distance(pos);}
	if ( c10def ) { nrvalidcoords++; totaldist+=c10.distance(pos);}
	if ( c01def ) { nrvalidcoords++; totaldist+=c01.distance(pos);}
	if ( c11def ) { nrvalidcoords++; totaldist+=c11.distance(pos);}

	if ( nrvalidcoords<3 ) continue;

	totaldist /=nrvalidcoords;
	if ( !isresset || totaldist<mindist )
	{
	    res = pid;
	    mindist = totaldist;
	    isresset = true;
	}
    }

    return isresset;
}


bool SurfaceGeometry::computeMeshNormal( Coord3& res, const PosID& pid,
				     const FloatMathFunction* t2dfunc ) const
{
    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( pid, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   t2dfunc );

    TypeSet<Coord3> normals;
    if ( c00def && c10def && c01def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c10def && c00def && c11def )
    {
	const Coord3 rowvec = c10-c00;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c01def && c00def && c11def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c01-c00;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    if ( c11def && c10def && c01def )
    {
	const Coord3 rowvec = c11-c01;
	const double rowveclen = rowvec.abs();
	if ( !mIsZero(rowveclen,mDefEps) )
	{
	    const Coord3 colvec = c11-c10;
	    const double colveclen = colvec.abs();
	    if ( !mIsZero(colveclen,mDefEps) )
		normals += rowvec.cross(colvec).normalize();
	}
    }

    res = estimateAverageVector( normals, false, false );
    return res.isDefined();
}


bool SurfaceGeometry::computeNormal( Coord3& res, const CubeSampling* cs,
		 const FloatMathFunction* t2dfunc, bool normalize ) const 
{
    TypeSet<PosID> nodes;
    if ( cs ) 
	findPos( *cs, &nodes );
    else
    {
	for ( int idy=0; idy<nrSections(); idy++ )
	{
	    const SectionID sectionid = sectionID(idy);
	    const int nrsections = nrSections();

	    StepInterval<int> rowrange; getRange( sectionid, rowrange, true );
	    if ( rowrange.width(false)<0 )
		continue;

	    StepInterval<int> colrange; getRange( sectionid, colrange, false );

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(sectionid,idx) )
			nodes += PosID(surface.id(),sectionid,rowCol2SubID(idx));
		}
	    }
	}
    }

    return computeNormal( res, nodes, t2dfunc, normalize );
}


#define mComputeNormalFetchNode(nodeindex) \
if ( !fetched[nodeindex] ) \
{ \
    fetched[nodeindex]=true; \
    const Coord3 tpos = getPos(getNeighbor(node,dirs[nodeindex])); \
    if ( tpos.isDefined() )  \
    { \
	while ( coords.size()<=nodeindex ) \
	    coords += Coord3::udf(); \
	coords[nodeindex] = Coord3(tpos,t2d ? t2d->getValue(tpos.z) : tpos.z); \
	defnodes[nodeindex] = true; \
    } \
}

bool SurfaceGeometry::computeNormal( Coord3& res, const PosID& node,
		     const FloatMathFunction* t2d, bool normalize ) const
{
    const Coord3 nodetpos = getPos(node);
    const bool defnode = nodetpos.isDefined();
    const Coord3 nodecoord( nodetpos,
	     		 t2d&&defnode ? t2d->getValue(nodetpos.z) : nodetpos.z);

    const TypeSet<RowCol>& dirs = RowCol::clockWiseSequence();
    BoolTypeSet defnodes(dirs.size(), false );
    BoolTypeSet fetched(dirs.size(), false );
    TypeSet<Coord3> coords;

    Coord3 rowvector;
    RowCol rowvecdir;
    bool validrowvector = false;
    Coord3 colvector;
    RowCol colvecdir;
    bool validcolvector = false;

    for ( int rowidx=0; rowidx<3; rowidx++ )
    {
	if ( validrowvector && validcolvector )
	    break;

	if ( !validrowvector )
	{
	    rowvecdir = RowCol(1,rowidx ? (rowidx==1?-1:1) : 0);
	    if ( validcolvector && rowvecdir==colvecdir )
		continue;

	    const int nextrowidx = dirs.indexOf(rowvecdir);
	    const int prevrowidx = dirs.indexOf(-rowvecdir);
	    mComputeNormalFetchNode(nextrowidx);
	    mComputeNormalFetchNode(prevrowidx);

	    if ( defnodes[nextrowidx] && defnodes[prevrowidx] )
	    {
		rowvector = coords[nextrowidx]-coords[prevrowidx];
		validrowvector = true;
	    }
	    else if ( defnode && defnodes[nextrowidx] )
	    {
		rowvector = coords[nextrowidx]-nodecoord;
		validrowvector = true;
	    }
	    else if ( defnode && defnodes[prevrowidx] )
	    {
		rowvector = nodecoord-coords[prevrowidx];
		validrowvector = true;
	    }

	    if ( validrowvector )
		rowvector=rowvector.normalize();
	}

	for ( int colidx=0; colidx<3; colidx++ )
	{
	    if ( !validcolvector )
	    {
		colvecdir = RowCol(colidx ? (colidx==1?-1:1) : 0, 1);
		if ( validrowvector && rowvecdir==colvecdir )
		    continue;
		const int nextcolidx = dirs.indexOf(colvecdir);
		const int prevcolidx = dirs.indexOf(-colvecdir);
		mComputeNormalFetchNode(nextcolidx);
		mComputeNormalFetchNode(prevcolidx);

		if ( defnodes[nextcolidx] && defnodes[prevcolidx] )
		{
		    colvector = coords[nextcolidx]-coords[prevcolidx];
		    validcolvector = true;
		}
		else if ( defnode && defnodes[nextcolidx] )
		{
		    colvector = coords[nextcolidx]-nodecoord;
		    validcolvector = true;
		}
		else if ( defnode && defnodes[prevcolidx] )
		{
		    colvector = nodecoord-coords[prevcolidx];
		    validcolvector = true;
		}

		if ( validcolvector )
		    colvector=colvector.normalize();
	    }
	}
    }

    if ( validcolvector && validrowvector )
    {
	res = normalize ? rowvector.cross(colvector).normalize() :
	    		  rowvector.cross(colvector);
	return true;
    }

    static const int prevrowidx = dirs.indexOf(RowCol(-1,0));
    mComputeNormalFetchNode(prevrowidx);
    static const int nextrowidx = dirs.indexOf(RowCol(1,0));
    mComputeNormalFetchNode(nextrowidx);

    if ( !validcolvector && defnode && defnodes[prevrowidx] &&
	    defnodes[nextrowidx] )
    {
	const Coord3 prevvector = (coords[prevrowidx]-nodecoord).normalize();
	const Coord3 nextvector = (coords[nextrowidx]-nodecoord).normalize();
	const Coord3 average = prevvector+nextvector;
	const double len = average.abs();
	if ( !mIsZero(len,mDefEps) )
	{
	    res = normalize ? average.normalize() :
			      average;
	    return true;
	}
    }

    static const int prevcolidx = dirs.indexOf(RowCol(0,-1));
    mComputeNormalFetchNode(prevcolidx);
    static const int nextcolidx = dirs.indexOf(RowCol(0,1));
    mComputeNormalFetchNode(nextcolidx);

    if ( !validrowvector && defnode && defnodes[prevcolidx] &&
	      defnodes[nextcolidx] )
    {
	const Coord3 prevvector = (coords[prevcolidx]-nodecoord).normalize();
	const Coord3 nextvector = (coords[nextcolidx]-nodecoord).normalize();
	const Coord3 average = prevvector+nextvector;
	const double len = average.abs();
	if ( !mIsZero(len,mDefEps) )
	{
	    res = normalize ? average.normalize() :
			      average;
	    return true;
	}
    }

    return false;
}


bool SurfaceGeometry::computeNormal( Coord3& res, const TypeSet<PosID>& nodes,
				 const FloatMathFunction* t2dfunc, bool normalize ) const
{
    TypeSet<Coord3> normals;
    const int nrnodes = nodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	const PosID& node = nodes[idx];
	Coord3 normal;
	if ( computeNormal(normal,nodes[idx],t2dfunc,false) )
	    normals += normal;
    }

    res = normalize ? estimateAverageVector(normals,false,false).normalize()
		    : estimateAverageVector(normals,false,false);
    return res.isDefined();
}


float SurfaceGeometry::normalDistance( const Coord3& timepos,
				   const FloatMathFunction* t2dfunc,
				   Interval<float>* meshvariation ) const
{
    PosID closestmesh(0,0,0);
    if ( !findClosestMesh(closestmesh,timepos,t2dfunc) )
	return mUndefValue;

    Coord3 meshnormal;
    if ( !computeMeshNormal(meshnormal,closestmesh,t2dfunc) )
	return mUndefValue;

    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( closestmesh, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   t2dfunc );

    Coord3 center(0,0,0);
    int nrvals = 0;
    if ( c00def ) {center+=c00; nrvals++;}
    if ( c10def ) {center+=c10; nrvals++;}
    if ( c01def ) {center+=c01; nrvals++;}
    if ( c11def ) {center+=c11; nrvals++;}
    center /= nrvals;

    const Plane3 plane( meshnormal, center, false );

    //Check how far the mesh's own coords are from the plane
    if ( meshvariation )
    {
	*meshvariation = Interval<float>(0,0);
	if ( c00def ) {meshvariation->include(plane.distanceToPoint(c00,true));}
	if ( c10def ) {meshvariation->include(plane.distanceToPoint(c10,true));}
	if ( c01def ) {meshvariation->include(plane.distanceToPoint(c01,true));}
	if ( c11def ) {meshvariation->include(plane.distanceToPoint(c11,true));}
    }

    const Coord3 pos = t2dfunc ? Coord3( timepos, t2dfunc->getValue(timepos.z) )
			       : timepos;

    const Line3 line( pos, meshnormal );
    Coord3 intersection;
    plane.intersectWith( line, intersection );
    const Coord3 vector = pos-intersection;
    return meshnormal.dot( vector );
}


char SurfaceGeometry::whichSide( const Coord3& timepos,
			     const FloatMathFunction* t2dfunc,
			     float fuzzy ) const
{
    Interval<float> meshvariation;
    const float dist = normalDistance( timepos, t2dfunc, &meshvariation);

    if ( dist>meshvariation.stop+fuzzy ) return 1;
    if ( dist<meshvariation.start-fuzzy ) return -1;
    return 0;
}


#define mGetNeigborCoord( coordname, defname, rowdiff, coldiff ) \
defname = false; \
for ( int idy=0; idy<nrnodealiases; idy++ ) \
{ \
    const PosID& nodealias = nodealiases[idy]; \
    const SectionID sectionid = nodealias.sectionID(); \
    const RowCol noderc = subID2RowCol(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    coordname = getPos(sectionid, neighborrc); \
    defname = coordname.isDefined(); \
    if ( defname ) \
    { \
	if ( t2dfunc ) \
	    coordname.z = t2dfunc->getValue(coordname.z); \
	break; \
    } \
} \



void SurfaceGeometry::getMeshCoords( const PosID& pid,
				 Coord3& c00, Coord3& c10, 
				 Coord3& c01, Coord3& c11,
				 bool& c00def, bool& c10def, 
				 bool& c01def, bool& c11def,
				 const FloatMathFunction* t2dfunc ) const
{
    TypeSet<PosID> nodealiases;
    getLinkedPos( pid, nodealiases );
    nodealiases += pid;
    const int nrnodealiases = nodealiases.size();

    c00 = getPos(pid);
    c00def = c00.isDefined();
    if ( c00def && t2dfunc ) c00.z = t2dfunc->getValue(c00.z);

    mGetNeigborCoord( c10, c10def, +step_.row, +0 );
    mGetNeigborCoord( c01, c01def, +0, +step_.col );
    mGetNeigborCoord( c11, c11def, +step_.row, +step_.col );
}


int SurfaceGeometry::nrSections() const
{
    return sectionids.size();
}


SectionID SurfaceGeometry::sectionID( int idx ) const
{
    return sectionids[idx];
}


SectionID SurfaceGeometry::sectionID( const char* nm ) const
{
    for ( int idx=0; idx<sectionnames.size(); idx++ )
	if ( *sectionnames[idx] == nm ) return sectionids[idx];
    return -1;
}


const char* SurfaceGeometry::sectionName( const SectionID& sectionid ) const
{
    int idx = sectionids.indexOf(sectionid);
    const char* res = idx!=-1 ? sectionnames[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


bool SurfaceGeometry::hasSection( const SectionID& sectionid ) const
{ return sectionNr(sectionid)!=-1; }


int SurfaceGeometry::sectionNr( const SectionID& sectionid ) const
{ return sectionids.indexOf(sectionid); }



SectionID SurfaceGeometry::addSection( const char* nm, bool addtohistory )
{
    SectionID res = 0;
    while ( sectionids.indexOf(res)!=-1 ) res++;

    addSection( nm, res, addtohistory );
    return res;
}


bool SurfaceGeometry::addSection( const char* nm, SectionID sectionid, 
			      bool addtohistory )
{
    if ( sectionids.indexOf(sectionid) != -1 ) return false;

    BufferString name;
    sectionids += sectionid;
    if ( nm && *nm )
	name = nm;
    else
	{ name = "["; name += sectionid + 1; name += "]"; }

    sectionnames += new BufferString(name);

    Geometry::MeshSurface* newsurf = createSectionSurface( sectionid );
    meshsurfaces += newsurf;

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfaceSectionEvent( true, surface.id(),
							sectionid, name );
	EM::EMM().history().addEvent( history, 0, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface.id(), sectionid, 0 );
    surface.notifier.trigger(cbdata);

    changed = true;
    return true;
}


void SurfaceGeometry::removeSection( SectionID sectionid, bool addtohistory )
{
    int idx=sectionids.indexOf(sectionid);
    if ( idx==-1 ) return;

    for ( int attr=0; attr<surface.nrPosAttribs(); attr++ )
    {
	const TypeSet<PosID>* attrset = surface.getPosAttribList( attr );
	if ( !attrset ) continue;
	for ( int idy=0; idy<attrset->size(); idy++ )
	{
	    const PosID& posid = (*attrset)[idy];
	    if ( sectionid == posid.sectionID() )
		surface.setPosAttrib( posid, attr, false );
	}
    }

    surface.edgelinesets.removeSection( sectionid );
    surface.relations.removeSection( sectionid );
    surface.auxdata.removeSection( sectionid );
    BufferString name = *sectionnames[idx];

    delete meshsurfaces[idx];
    meshsurfaces.remove( idx );
    sectionids.remove( idx );
    sectionnames.remove( idx );

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfaceSectionEvent( false, surface.id(),
							sectionid, name );
	EM::EMM().history().addEvent( history, 0, 0 );
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface.id(), sectionid, 0 );
    surface.notifier.trigger(cbdata);

    changed = true;
}


SectionID SurfaceGeometry::cloneSection( SectionID sectionid )
{
    int sectionidx = sectionids.indexOf(sectionid);
    if ( sectionidx==-1 ) return -1;

    SectionID res = addSection(0, true);
    StepInterval<int> rowrange;
    StepInterval<int> colrange;
    getRange( sectionid, rowrange, true );
    if ( rowrange.width(false)>=0 )
	getRange( sectionid, colrange, false );

    for ( int row=rowrange.start; row<=rowrange.stop; row+=step_.row )
    {
	for ( int col=colrange.start; col<=colrange.stop; col+=step_.col )
	{
	    const RowCol rc(row,col);
	    const Coord3 pos = getPos(sectionid,rc);
	    if ( !pos.isDefined() )
		continue;

	    setPos(res,rc,pos,false, true);

	    const PosID src(surface.id(),sectionid,rowCol2SubID(rc));
	    const PosID dst(surface.id(),res,rowCol2SubID(rc));
	    for ( int idy=0; idy<surface.nrPosAttribs(); idy++ )
	    {
		const int attrib = surface.posAttrib(idy);
		if ( surface.isPosAttrib( src, attrib ) )
		    surface.setPosAttrib( dst, attrib, true );
	    }
	}
    }

    surface.edgelinesets.cloneEdgeLineSet( sectionid, res );
    surface.relations.cloneSectionRelation( sectionid, res );

    return res;
}


bool SurfaceGeometry::setPos( const SectionID& section, const RowCol& surfrc,
			  const Coord3& pos, bool autoconnect,
			  bool addtohistory)
{
    RowCol geomrowcol;
    if ( !getMeshRowCol( surfrc, geomrowcol, section ) )
	return false;

    int sectionindex=sectionids.indexOf(section);
    if ( sectionindex==-1 ) return false;

    const Geometry::PosID posid = Geometry::MeshSurface::getPosID(geomrowcol);
    Geometry::MeshSurface* meshsurf = meshsurfaces[sectionindex];
    const Coord3 oldpos = meshsurf->getMeshPos( geomrowcol );

    if ( addtohistory )
    {
	PosID pid( surface.id(), section, rowCol2SubID(surfrc) );
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pid );
	EM::EMM().history().addEvent( history, 0, 0 );
    }


    if ( oldpos==pos ) return true;

    changed = true;

    TypeSet<PosID> nodeonothersections;
    if ( autoconnect )
	findPos( geomrowcol, nodeonothersections );

    meshsurf->setMeshPos( geomrowcol, pos );
    meshsurf->setFillType( geomrowcol, Geometry::MeshSurface::Filled );

    if ( !pos.isDefined() )
	meshsurf->shrink();

    if ( autoconnect )
    {
	for ( int idx=0; idx<nodeonothersections.size(); idx++ )
	{
	    const int sectionsurfidx =
		sectionids.indexOf(nodeonothersections[idx].sectionID());
	    double otherz = meshsurfaces[sectionsurfidx]->getMeshPos(geomrowcol).z;
	    
	    if ( mIsEqual(otherz,pos.z,mDefEps) )
	    {
		if ( !meshsurf->isLinked(posid,meshsurfaces[sectionsurfidx],posid) )
		{
		    meshsurf->setLink(posid,meshsurfaces[sectionsurfidx],posid,true);
		    // Put to history?
		}
	    }
	}
    }


    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::PositionChange;
    cbdata.pid0 = PosID(surface.id(),section,rowCol2SubID(surfrc));
    surface.notifier.trigger( cbdata, &surface );

    return true;
}


bool SurfaceGeometry::setPos( const PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.objectID()!=surface.id() ) return false;

    return setPos( posid.sectionID(), subID2RowCol(posid.subID()),
	    	   newpos, false, addtohistory );
}


Coord3 SurfaceGeometry::getPos( const PosID& posid ) const
{
    return getPos( posid.sectionID(), subID2RowCol(posid.subID()) );
}


Coord3 SurfaceGeometry::getPos( const SectionID& section, const RowCol& rc) const
{
    const int surfidx = sectionids.indexOf( section );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, section ) )
	return Coord3::udf();

    return meshsurfaces[surfidx]->getMeshPos( geomnode );
}


void SurfaceGeometry::getPos( const RowCol& rc, TypeSet<Coord3>& crdset ) const
{
    const int nrsubsurf = nrSections();
    for ( int surfidx=0; surfidx<nrsubsurf; surfidx++ )
    {
	Coord3 crd = getPos( sectionID(surfidx), rc );
	if ( crd.isDefined() )
	    crdset += crd;
    }
}


bool SurfaceGeometry::isDefined( const PosID& posid ) const
{
    return isDefined( posid.sectionID(), subID2RowCol(posid.subID()) );
}


bool SurfaceGeometry::isDefined( const SectionID& section, const RowCol& rc ) const
{
    const int surfidx = sectionids.indexOf( section );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, section ) )
	return false;

    return meshsurfaces[surfidx]->isDefined( geomnode );
}


int SurfaceGeometry::findPos( const RowCol& rowcol, TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrSections();
    for ( SectionID section=0; section<nrsubsurf; section++ )
    {
	Geometry::MeshSurface* meshsurf = meshsurfaces[section];
	if ( !meshsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = meshsurf->getMeshPos( rowcol );
	SubID subid = rowCol2SubID( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIsEqual(respos[idx].z,pos.z,mDefEps) ) continue;

	    res += PosID(surface.id(), sectionID(section), subid );
	    respos += pos;
	}
    }

    return res.size();
}


int SurfaceGeometry::findPos( const SectionID& sectionid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<PosID>* res ) const	
{
    int idx = sectionids.indexOf(sectionid);
    if ( idx<0 ) return 0;

    TypeSet<PosID> posids;
    TypeSet<Geometry::PosID> nodes;
    meshsurfaces[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width()/2, y.width()/2, z.width()/2, nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const SectionID section = sectionids[idx];
	const PosID posid( surface.id(), sectionid, 
			getSurfSubID(nodes[idy],sectionid) );

	TypeSet<PosID> clones;
	getLinkedPos( posid, clones );
	clones += posid;

	const int nrclones = clones.size();
	bool found = false;
	for ( int idz=0; idz<nrclones; idz++ )
	{
	    if ( posids.indexOf(clones[idz]) != -1 )
	    { found = true; break; }
	}

	if ( !found )
	    posids += posid;
    }

    if ( res ) res->append(posids);
    return posids.size();
}


int SurfaceGeometry::findPos( const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<PosID>* res ) const	
{
    int sum = 0;
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
	sum += findPos( sectionID(idx), x, y, z, res );

    return sum;
}

    
int SurfaceGeometry::findPos( const CubeSampling& cs,
			  TypeSet<PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hrg.start);
    Interval<float> xinterval( xypos.x, xypos.x );
    Interval<float> yinterval( xypos.y, xypos.y );

    xypos = SI().transform(cs.hrg.stop);
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    xypos = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );
    xinterval.include( xypos.x );
    yinterval.include( xypos.y );

    TypeSet<PosID> posids;
    findPos( xinterval, yinterval, cs.zrg, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(getPos(posid));

	if ( nodebid.inl<cs.hrg.start.inl || nodebid.inl>cs.hrg.stop.inl ||
	     nodebid.crl<cs.hrg.start.crl || nodebid.crl>cs.hrg.stop.crl )
	{
	    posids.removeFast( idx-- );
	    continue;
	}
    }

    if ( res ) res->append(posids);
    return posids.size();
}


PosID SurfaceGeometry::getNeighbor( const PosID& posid,
				    const RowCol& dir ) const
{
    RowCol diff(0,0);
    if ( dir.row>0 ) diff.row = step_.row;
    else if ( dir.row<0 ) diff.row = -step_.row;

    if ( dir.col>0 ) diff.col = step_.col;
    else if ( dir.col<0 ) diff.col = -step_.col;
    
    TypeSet<PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc = subID2RowCol(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( isDefined(aliases[idx].sectionID(),neigborrc) )
	    return PosID( surface.id(), aliases[idx].sectionID(),
		    	      rowCol2SubID(neigborrc));
    }

    const RowCol ownrc = subID2RowCol(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return PosID( surface.id(), posid.sectionID(), rowCol2SubID(neigborrc));
}


int SurfaceGeometry::getNeighbors( const PosID& posid_, TypeSet<PosID>* res,
			       int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<PosID> > neigbors;
    const RowCol start = subID2RowCol(posid_.subID());
    neigbors += new TypeSet<PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol = subID2RowCol(currentposid.subID());

	    for ( int row=-step_.row; row<=step_.row; row+=step_.row )
	    {
		for ( int col=-step_.col; col<=step_.col; col+=step_.col )
		{
		    if ( !row && !col ) continue;

		    const RowCol neighborrowcol(rowcol.row+row,rowcol.col+col);
		    const int drow =abs(neighborrowcol.row-start.row)/step_.row;
		    const int dcol =abs(neighborrowcol.col-start.col)/step_.col;

		    if ( drow>maxradius || dcol>maxradius )
			continue;

		    if ( circle && (drow*drow+dcol*dcol)> maxradius*maxradius)
			continue;

		    if ( !isDefined(currentposid.sectionID(),neighborrowcol) )
			continue;
		   
		    const PosID
			    neighborposid(currentposid.objectID(),
			    currentposid.sectionID(),
			    rowCol2SubID(neighborrowcol) );

		    bool found = false;
		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<PosID>& posids = *new TypeSet<PosID>;
		    getLinkedPos( neighborposid, posids );
		    posids.insert( 0, neighborposid );

		    neigbors += &posids;

		}
	    }
	}
    }

    if ( res )
    {
	// Leave out the fist one, since it's the origin
	for ( int idx=1; idx<neigbors.size(); idx++ )
	{
	    (*res) += (*neigbors[idx])[0];
	}
    }

    const int size = neigbors.size();
    deepErase( neigbors );

    // Leave out the fist one, since it's the origin
    return size-1;
}


void SurfaceGeometry::getLinkedPos( const PosID& posid,
				TypeSet<PosID>& res ) const
{
    if ( posid.objectID()!=surface.id() )
        return; //TODO: Implement handling for this case

    const SubID subid = posid.subID();
    const RowCol rowcol = subID2RowCol(subid);
    const Geometry::MeshSurface* ownmeshsurf = getSurface( posid.sectionID() );
    if ( !ownmeshsurf ) return;

    const int nrsubsurf = nrSections();
    for ( int section=0; section<nrsubsurf; section++ )
    {
	Geometry::MeshSurface* meshsurf = meshsurfaces[section];
	if ( ownmeshsurf->isLinked( subid, meshsurf, subid ) )
	{
	    res += PosID( surface.id(),sectionids[section], subid );
	}
    }
}


bool SurfaceGeometry::isLoaded() const
{
    return nrSections();
}


RowCol SurfaceGeometry::loadedStep() const
{
    return loadedstep;
}


RowCol SurfaceGeometry::step() const
{
    return step_;
}


void SurfaceGeometry::setTranslatorData( const RowCol& step__,
				     const RowCol& loadedstep_,
				     const RowCol& origo_,
				     const Interval<int>* rowrange_,
				     const Interval<int>* colrange_ )
{
    step_ = step__;
    loadedstep = loadedstep_;
    origos += origo_;
    delete rowinterval;
    delete colinterval;
    rowinterval = rowrange_ ? new Interval<int>( *rowrange_ ) : 0;
    colinterval = colrange_ ? new Interval<int>( *colrange_ ) : 0;
}


RowCol SurfaceGeometry::subID2RowCol( const SubID& subid )
{
    return int642rc(subid);
}


SubID SurfaceGeometry::rowCol2SubID( const RowCol& rc )
{
    return rc2int64(rc);
}


bool SurfaceGeometry::isFullResolution() const
{
    return loadedstep == step_;
}


bool SurfaceGeometry::getMeshRowCol( const SubID& subid, RowCol& meshrowcol, 
				 const SectionID& sectionid ) const
{
    return getMeshRowCol( subID2RowCol(subid), meshrowcol, sectionid );
}


bool SurfaceGeometry::getMeshRowCol( const RowCol& emrowcol, RowCol& meshrowcol,
       				 const SectionID& sectionid ) const
{
    const int idx = sectionids.indexOf( sectionid );
    RowCol origo = idx<origos.size() ? origos[idx] :
		   (origos.size() ? origos[0] : RowCol(0,0) );
    const RowCol relrowcol = emrowcol - origo;
    if ( relrowcol.row%loadedstep.row || relrowcol.col%loadedstep.col )
	return false;

    meshrowcol = relrowcol/loadedstep;
    return true;
}


SubID SurfaceGeometry::getSurfSubID( const RowCol& nodeid, 
				     const SectionID& sectionid ) const
{
    const int idx = sectionids.indexOf( sectionid );
    RowCol origo = idx<origos.size() ? origos[idx] :
	(origos.size() ? origos[0] : RowCol(0,0) );
    return rowCol2SubID( origo+nodeid*loadedstep );
}


SubID SurfaceGeometry::getSurfSubID( const Geometry::PosID& gposid,
       				     const SectionID& sectionid ) const
{
    const RowCol& nodeid = Geometry::MeshSurface::getMeshNode(gposid);
    return getSurfSubID( nodeid, sectionid );
}



const Geometry::MeshSurface* SurfaceGeometry::getSurface( SectionID sectionid )const
{
    const int idx = sectionids.indexOf( sectionid );
    return idx==-1 ? 0 : meshsurfaces[idx];
}


void SurfaceGeometry::getRange( StepInterval<int>& rg, bool rowdir ) const
{
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	const SectionID sectionid = sectionID( idx );
	StepInterval<int> sectionrg;
	getRange( sectionID(idx), sectionrg, rowdir );
	
	if ( !idx )
	    rg = sectionrg;
	else
	{
	    rg.include( sectionrg.start ); 
	    rg.include( sectionrg.stop );
	}
    }
}


void SurfaceGeometry::getRange( const SectionID& sectionid, 
			    StepInterval<int>& rg,
			    bool rowdir ) const
{
    const Geometry::MeshSurface& gsurf = *getSurface( sectionid );
    if ( rowdir )
    {
	const RowCol firstrow(gsurf.firstRow(),0);
	const RowCol lastrow(gsurf.lastRow(),0);

	rg.start = subID2RowCol( getSurfSubID(firstrow,sectionid) ).row;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,sectionid) ).row;
    }
    else
    {
	const Interval<int> colrg = gsurf.getColInterval();
	const RowCol firstrow(0,colrg.start);
	const RowCol lastrow(0,colrg.stop);

	rg.start = subID2RowCol( getSurfSubID(firstrow,sectionid) ).col;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,sectionid) ).col;
    }

    rg.step = rowdir ? loadedStep().row : loadedStep().col;
}


bool SurfaceGeometry::isAtEdge( const PosID& pid ) const
{
    if ( !isDefined(pid) ) return false;

    return !isDefined(getNeighbor(pid,RowCol(0,1))) ||
	     !isDefined(getNeighbor(pid,RowCol(1,0))) ||
	     !isDefined(getNeighbor(pid,RowCol(0,-1))) ||
	     !isDefined(getNeighbor(pid,RowCol(-1,0)));
}


Executor* SurfaceGeometry::loader( const SurfaceIODataSelection* newsel )
{
    PtrMan<IOObj> ioobj = IOM().get( surface.multiID() );
    if ( !ioobj )
	{ surface.errmsg = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	{ surface.errmsg = tr ? tr->errMsg() :
	    "Cannot find Translator"; return 0; }

    SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }
    else
	sel.selvalues.erase();

    Executor* exec = tr->reader( surface );
    surface.errmsg = tr->errMsg();
    return exec;
}


Executor* SurfaceGeometry::saver( const SurfaceIODataSelection* newsel,
       			      const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : surface.multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	{ surface.errmsg = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startWrite(surface) )
	{ surface.errmsg = tr ? tr->errMsg() : "No Translator"; return 0; }

    SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }

    Executor* exec = tr->writer(*ioobj);
    surface.errmsg = tr->errMsg();
    return exec;
}


bool SurfaceGeometry::usePar( const IOPar& par )
{
    return true;
}


void SurfaceGeometry::fillPar( IOPar& par ) const
{ }

}; //namespace

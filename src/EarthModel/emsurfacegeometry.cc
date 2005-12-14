/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Nov 2002
 RCS:           $Id: emsurfacegeometry.cc,v 1.29 2005-12-14 16:47:50 cvskris Exp $
________________________________________________________________________

-*/

#include "emsurfacegeometry.h"

#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfacerelations.h"
#include "emsurfaceauxdata.h"

#include "emsurfaceedgeline.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "parametricsurface.h"
#include "mathfunc.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "pca.h"
#include "toplist.h"
#include "survinfo.h"

namespace EM {


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


class SurfaceSectionHistoryEvent : public HistoryEvent
{
public:
			SurfaceSectionHistoryEvent( bool add,
				ObjectID, const SectionID&,
				const char* name );
    const char*         getStandardDesc() const;
    bool                unDo();
    bool                reDo();
    void                fillPar(IOPar&) const;
    bool                usePar(const IOPar&);

protected:
    bool				action(bool add) const;

    bool				add;
    ObjectID				object;
    SectionID				sid;
    BufferString			name;

    static const char*  		addKey();
    static const char*  		objKey();
    static const char*  		sectionKey();
    static const char*  		nameKey();
};


SurfaceSectionHistoryEvent::SurfaceSectionHistoryEvent(
	bool doadd, ObjectID oid,
        const SectionID& sectionid, const char* nm)
    : object( oid )
    , sid( sid )
    , name( nm )
    , add( doadd )
{}


const char* SurfaceSectionHistoryEvent::getStandardDesc() const
{
    return "Modified surface section";
}


bool SurfaceSectionHistoryEvent::unDo()
{
    return action( !add );
}


bool SurfaceSectionHistoryEvent::reDo()
{
    return action( add );
}


const char* SurfaceSectionHistoryEvent::addKey() { return "Add"; }
const char* SurfaceSectionHistoryEvent::objKey() { return "Object"; }
const char* SurfaceSectionHistoryEvent::sectionKey() { return "Section"; }
const char* SurfaceSectionHistoryEvent::nameKey() { return "Name"; }


void SurfaceSectionHistoryEvent::fillPar( IOPar& iopar ) const
{
    iopar.setYN( addKey(), add );
    iopar.set( objKey(), object );
    iopar.set( sectionKey(), (int) sid );
    if ( add ) iopar.set( nameKey(), name );
}


bool SurfaceSectionHistoryEvent::usePar( const IOPar& iopar )
{
    int tmpsection;
    bool res = iopar.getYN( addKey(), add ) && iopar.get( objKey(), object )
	    && iopar.get( sectionKey(), tmpsection );
    if ( res )
    {
	if ( add )
	    res = iopar.get( nameKey(), name );

	if ( res )
	    sid = tmpsection;
    }

    return res;
}


bool SurfaceSectionHistoryEvent::action( bool doadd ) const
{
    EMManager& manager = EMM();
    EMObject* objectptr = manager.getObject(object);
    Surface* emsurface = dynamic_cast<Surface*>(objectptr);

    if ( doadd )
	return emsurface->geometry.addSection( name, sid, false );

    emsurface->geometry.removeSection( sid, false );
    return true;
}


SurfaceGeometry::SurfaceGeometry( Surface& surf_ )
    : step_(SI().inlStep(),SI().crlStep())
    , loadedstep_(SI().inlStep(),SI().crlStep())
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
    while ( nrSections() )
	removeSection( sectionID(0), false );
}


void SurfaceGeometry::checkSections()
{
    const int nrsections = nrSections();
    for ( int sidx=0; sidx<nrsections; sidx++ )
    {
	const SectionID sid = sectionID( sidx );
	const Geometry::ParametricSurface* psurf = getSurface( sid );
	if ( !psurf ) return;

	bool isundef = true;
	const int nrnodes = psurf->nrKnots();
	for ( int nodeidx=0; nodeidx<nrnodes; nodeidx++ )
	{
	    const RowCol rc = psurf->getKnotRowCol( nodeidx );
	    if ( psurf->isKnotDefined(rc) )
	    {
		isundef = false;
		break;
	    }
	}

	if ( isundef )
	    removeSection( sid, false );
    }
}


bool SurfaceGeometry::findClosestNodes( TopList<float,PosID>& toplist,
				    const Coord3& pos_,
				    const FloatMathFunction* t2dfunc) const
{
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
	findClosestNodes( sectionID(idx), toplist, pos_, t2dfunc );

    return toplist.size();
}


bool SurfaceGeometry::findClosestNodes( const SectionID& sid,
				    TopList<float,PosID>& toplist,
				    const Coord3& pos_,
				    const FloatMathFunction* t2dfunc ) const
{
    toplist.setTop(false);

    //TODO Make faster impl
    Coord3 origpos = pos_;
    if ( t2dfunc ) origpos.z = t2dfunc->getValue( pos_.z );

    const StepInterval<int> rowrange = rowRange(); 
    if ( rowrange.width(false)<0 )
	return false;
    const StepInterval<int> colrange = rowRange();

    RowCol rc;
    for ( rc.row=rowrange.start;rc.row<=rowrange.stop;rc.row+=rowrange.step)
    {
	for ( rc.col=colrange.start; rc.col<=colrange.stop;
						    rc.col+=colrange.step )
	{
	    if ( isDefined(sid,rc) )
	    {
		Coord3 pos = surface.getPos( sid, rc.getSerialized() );
		if ( t2dfunc )
		    pos.z = t2dfunc->getValue( pos.z );

		double dist = pos.distance( origpos );
		toplist.addValue( dist,
			      PosID(surface.id(),sid,rc.getSerialized()) );
	    
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
	    const SectionID sid = sectionID(idy);
	    const int nrsections = nrSections();

	    const StepInterval<int> rowrange = rowRange(sid); 
	    if ( rowrange.width(false)<0 )
		continue;

	    const StepInterval<int> colrange = colRange(sid);

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(sid,idx) )
			nodes += PosID(surface.id(),sid,idx.getSerialized());
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
    const Coord3 tpos = surface.getPos(getNeighbor(node,dirs[nodeindex])); \
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
    const Coord3 nodetpos = surface.getPos(node);
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


bool SurfaceGeometry::checkSupport( bool yn )
{
    const bool res = checksSupport();
    for ( int idx=0; idx<meshsurfaces.size(); idx++ )
    	meshsurfaces[idx]->checkSupport(yn);

    return res;
}


bool SurfaceGeometry::checksSupport() const
{ return meshsurfaces.size() ? meshsurfaces[0]->checksSupport() : true; }


bool SurfaceGeometry::checkSelfIntersection( bool yn )
{
    const bool res = checksSelfIntersection();
    for ( int idx=0; idx<meshsurfaces.size(); idx++ )
    	meshsurfaces[idx]->checkSelfIntersection(yn);

    return res;
}


bool SurfaceGeometry::checksSelfIntersection() const
{
    return meshsurfaces.size()
	? meshsurfaces[0]->checksSelfIntersection()
	: true;
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
    const SectionID sid = nodealias.sectionID(); \
    const RowCol noderc(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    coordname = surface.getPos(sid, neighborrc.getSerialized()); \
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

    c00 = surface.getPos(pid);
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


const char* SurfaceGeometry::sectionName( const SectionID& sid ) const
{
    int idx = sectionids.indexOf(sid);
    const char* res = idx!=-1 ? sectionnames[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


bool SurfaceGeometry::setSectionName( const SectionID& sid, const char* nm,
				      bool addtohistory )
{
    const int idx = sectionids.indexOf(sid);
    if ( !nm || !*nm || idx<0 )
	return false;

    sectionnames.get(idx) = nm;

    if ( addtohistory )
	pErrMsg("Section namechange history not implemented" );

    changed = true;
    return true;
}


bool SurfaceGeometry::hasSection( const SectionID& sid ) const
{ return sectionNr(sid)!=-1; }


int SurfaceGeometry::sectionNr( const SectionID& sid ) const
{ return sectionids.indexOf(sid); }



SectionID SurfaceGeometry::addSection( const char* nm, bool addtohistory )
{
    SectionID res = 0;
    while ( sectionids.indexOf(res)!=-1 ) res++;

    return addSection( nm, res, addtohistory );
}


SectionID SurfaceGeometry::addSection( const char* nm, const SectionID& sid, 
				       bool addtohistory )
{
    Geometry::ParametricSurface* newsurf = createSectionSurface();
    return newsurf ? addSection( newsurf, nm, sid, addtohistory ) : -1;
}


SectionID SurfaceGeometry::addSection( Geometry::ParametricSurface* surf,
				   const char* nm, const SectionID& wantedsid,
				   bool addtohistory )
{
    if ( !surf ) return -1;
    surf->checkSupport( checksSupport() );

    SectionID sid = wantedsid;
    if ( sid==-1 )
    {
	sid = 0;
	while ( sectionids.indexOf(sid)!=-1 ) sid++;
    }
    
    BufferString name; 
    if ( nm && *nm ) name = nm;
    else { name = "["; name += sid + 1; name += "]"; }

    sectionids += sid;
    sectionnames += new BufferString(name);
    meshsurfaces += surf;

    if ( addtohistory )
    {
	pErrMsg("History not implemented for add section");
	EMM().history().setCurrentEventAsFirst();
	/*
	HistoryEvent* history =
	    new SurfaceSectionHistoryEvent( true, surface.id(), sid, name );
	EMM().history().addEvent( history, 0, 0 );
	*/
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface.id(), sid, 0 );
    surface.notifier.trigger(cbdata);

    changed = true;
    return sid;
}


void SurfaceGeometry::removeSection( const SectionID& sid, bool addtohistory )
{
    int idx=sectionids.indexOf(sid);
    if ( idx==-1 ) return;

    for ( int attr=0; attr<surface.nrPosAttribs(); attr++ )
    {
	const TypeSet<PosID>* attrset = surface.getPosAttribList( attr );
	if ( !attrset ) continue;
	for ( int idy=0; idy<attrset->size(); idy++ )
	{
	    const PosID& posid = (*attrset)[idy];
	    if ( sid == posid.sectionID() )
		surface.setPosAttrib( posid, attr, false );
	}
    }

    surface.edgelinesets.removeSection( sid );
    surface.relations.removeSection( sid );
    surface.auxdata.removeSection( sid );
    BufferString name = *sectionnames[idx];

    delete meshsurfaces[idx];
    meshsurfaces.remove( idx );
    sectionids.remove( idx );
    sectionnames.remove( idx );

    if ( addtohistory )
    {
	pErrMsg("History not implemented for remove section");
	EMM().history().setCurrentEventAsFirst();
	/*

	HistoryEvent* history =
	    new SurfaceSectionHistoryEvent( false, surface.id(),
					    sid, name );
	EMM().history().addEvent( history, 0, 0 );
	*/
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface.id(), sid, 0 );
    surface.notifier.trigger(cbdata);

    changed = true;
}


SectionID SurfaceGeometry::cloneSection( const SectionID& sid )
{
    int sectionidx = sectionids.indexOf(sid);
    if ( sectionidx==-1 ) return -1;

    Geometry::ParametricSurface* newsurf = meshsurfaces[sectionidx]->clone();
    SectionID res = addSection( newsurf, 0, -1, true );

    surface.edgelinesets.cloneEdgeLineSet( sid, res );
    surface.relations.cloneSectionRelation( sid, res );

    return res;
}

bool SurfaceGeometry::insertRowOrCol( const SectionID& sid, int rc, bool row,
				      bool hist )
{
    const bool didcheck = checkSelfIntersection(false);
    const int dim = row ? 0 : 1;

    TypeSet<RowCol> movedrc;
    TypeSet<RowCol> queue;

    PtrMan<EMObjectIterator> iterator = surface.createIterator(sid);
    while ( iterator )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const RowCol fromrc( pid.subID() );
	if ( fromrc[dim]<rc )
	    continue;

	queue += fromrc;
    }

    mDynamicCastGet( Geometry::ParametricSurface*, paramsurf,
	    	     surface.getElementInternal(sid) );
    const StepInterval<int> perprg = row ? colRange(sid) : rowRange(sid);
    TypeSet<RowCol> replacercs;
    TypeSet<Coord3> replacepos;
    for ( int idx=perprg.start; idx<=perprg.stop; idx+= perprg.step )
    {
	const RowCol replacerc( row ? rc : idx, row ? idx : rc );
	Coord param( replacerc.row, replacerc.col );
	param[dim] -= (float) step()[dim]/2;
	
	replacercs += RowCol( row ? rc : idx, row ? idx : rc );
	replacepos += paramsurf->computePosition(param);
    }

    while ( queue.size() )
    {
	for ( int idx=queue.size()-1; idx>=0; idx-- )
	{
	    const RowCol& fromrc = queue[idx];
	    RowCol torc( fromrc );
	    torc[dim] = fromrc[dim]+step_[dim];

	    if ( isDefined(sid,torc) && movedrc.indexOf(torc)==-1 )
		continue;

	    const EM::PosID frompid( surface.id(), sid, fromrc.getSerialized());
	    const EM::PosID topid( surface.id(), sid, torc.getSerialized());
	    surface.changePosID( frompid, topid, hist );
	    queue.removeFast(idx);
	    movedrc += fromrc;
	}
    }

    for ( int idx=replacercs.size()-1; idx>=0; idx-- )
	paramsurf->setKnot(replacercs[idx],replacepos[idx]);

    checkSelfIntersection(didcheck);
    return true;
}


bool SurfaceGeometry::insertRow( const SectionID& sid, int rc, bool hist )
{
    return insertRowOrCol( sid, rc, true, hist );
}


bool SurfaceGeometry::insertCol( const SectionID& sid, int rc, bool hist )
{
    return insertRowOrCol( sid, rc, false, hist );
}


void SurfaceGeometry::getPos( const RowCol& rc, TypeSet<Coord3>& crdset ) const
{
    const int nrsubsurf = nrSections();
    for ( int surfidx=0; surfidx<nrsubsurf; surfidx++ )
    {
	Coord3 crd = surface.getPos( sectionID(surfidx), rc.getSerialized() );
	if ( crd.isDefined() )
	    crdset += crd;
    }
}


bool SurfaceGeometry::isDefined( const PosID& posid ) const
{
    return isDefined( posid.sectionID(), RowCol(posid.subID()) );
}


bool SurfaceGeometry::isDefined( const SectionID& sid, const RowCol& rc ) const
{
    const int surfidx = sectionids.indexOf( sid );
    if ( surfidx < 0 || surfidx >= meshsurfaces.size() ) return false;

    return meshsurfaces[surfidx]->isKnotDefined( rc );
}


int SurfaceGeometry::findPos( const RowCol& rowcol, TypeSet<PosID>& res ) const
{
    const int nrsubsurf = nrSections();
    for ( int idx=0; idx<nrsubsurf; idx++ )
    {
	Geometry::ParametricSurface* meshsurf = meshsurfaces[idx];
	if ( !meshsurf->isKnotDefined( rowcol ) )
	    continue;

	Coord3 pos = meshsurf->getKnot( rowcol );
	SubID subid = rowcol.getSerialized();

	res += PosID( surface.id(), sectionID(idx), subid );
    }

    return res.size();
}


int SurfaceGeometry::findPos( const SectionID& sid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<PosID>* res ) const	
{
    /*
    int idx = sectionids.indexOf(sid);
    if ( idx<0 ) return 0;

    TypeSet<PosID> posids;
    TypeSet<GeomPosID> nodes;
    meshsurfaces[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width()/2, y.width()/2, z.width()/2, nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const SectionID sid = sectionids[idx];
	const PosID posid( surface.id(), sid, 
			getSurfSubID(nodes[idy],sid) );

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
    */
    return 0;
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
	const BinID nodebid = SI().transform(surface.getPos(posid));

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
	const RowCol ownrc(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( isDefined(aliases[idx].sectionID(),neigborrc) )
	    return PosID( surface.id(), aliases[idx].sectionID(),
		    	  neigborrc.getSerialized() );
    }

    const RowCol ownrc(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return PosID( surface.id(), posid.sectionID(), neigborrc.getSerialized());
}


int SurfaceGeometry::getNeighbors( const PosID& posid_, TypeSet<PosID>* res,
			       int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<PosID> > neigbors;
    const RowCol start(posid_.subID());
    neigbors += new TypeSet<PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    PosID currentposid = (*neigbors[idx])[idz];
	    const RowCol rowcol(currentposid.subID());

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
			    neighborrowcol.getSerialized() );

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
    const RowCol rowcol(subid);
    const Geometry::ParametricSurface* ownmeshsurf =
				getSurface( posid.sectionID() );
    if ( !ownmeshsurf ) return;
}


bool SurfaceGeometry::isLoaded() const
{
    return nrSections();
}


RowCol SurfaceGeometry::loadedStep() const
{
    return loadedstep_;
}


RowCol SurfaceGeometry::step() const
{
    return step_;
}


void SurfaceGeometry::setStep( const RowCol& ns, const RowCol& loadedstep )
{
    step_ = ns;
    loadedstep_ = loadedstep;
}


bool SurfaceGeometry::isFullResolution() const
{
    return loadedstep_ == step_;
}


const Geometry::ParametricSurface*
    SurfaceGeometry::getSurface( const SectionID& sid ) const
{
    const int idx = sectionids.indexOf( sid );
    return idx==-1 ? 0 : meshsurfaces[idx];
}

#define mGetRange( funcname ) \
StepInterval<int> SurfaceGeometry::funcname(const SectionID& sid) const \
{ \
    if ( sid==-1 ) \
    { \
	StepInterval<int> res(0,0,0); \
	bool isset = false; \
	for ( int idx=0; idx<nrSections(); idx++ ) \
	{ \
	    Geometry::ParametricSurface* surf = meshsurfaces[idx]; \
	    if ( !surf->nrKnots() ) continue; \
 \
	    StepInterval<int> sectionrg = surf->funcname(); \
 \
	    if ( !isset ) { res = sectionrg; isset=true; } \
	    else res.include( sectionrg );  \
	} \
 \
	return res; \
    } \
 \
    StepInterval<int> res(0,0,0); \
    const Geometry::ParametricSurface* gsurf = getSurface( sid ); \
    if ( gsurf ) res = gsurf->funcname(); \
    return res; \
}


mGetRange( rowRange )
mGetRange( colRange )


Interval<float> SurfaceGeometry::getZRange( const Interval<int>& rowrg,
					    const Interval<int>& colrg ) const
{
    Interval<float> zrg( mUdf(float), -mUdf(float) );
    for ( int sidx=0; sidx<nrSections(); sidx++ )
    {
	const SectionID sid = sectionID( sidx );
	const Geometry::ParametricSurface* psurf = getSurface( sid );
	for ( int nidx=0; nidx<psurf->nrKnots(); nidx++ )
	{
	    RowCol rc = psurf->getKnotRowCol( nidx );
	    if ( rowrg.includes(rc.r()) && colrg.includes(rc.c()) )
	    {
		const Coord3& crd = surface.getPos( sid, rc.getSerialized() );
		if ( crd.isDefined() ) zrg.include( crd.z, false );
	    }
	}
    }

    return zrg;
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

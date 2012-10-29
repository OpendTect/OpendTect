/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emsurfacegeometry.h"

#include "emrowcoliterator.h"

#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"

#include "emsurfaceedgeline.h"
#include "undo.h"
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


class SurfaceSectionUndoEvent : public UndoEvent
{
public:
			SurfaceSectionUndoEvent( bool add,
				ObjectID, const SectionID&,
				const char* name );
    const char*         getStandardDesc() const;
    bool                unDo();
    bool                reDo();
    void                fillPar(IOPar&) const;
    bool                usePar(const IOPar&);

protected:
    bool				action(bool add) const;

    bool				add_;
    ObjectID				object_;
    SectionID				sid_;
    BufferString			name_;

    static const char*  		addKey();
    static const char*  		objKey();
    static const char*  		sectionKey();
    static const char*  		nameKey();
};


SurfaceSectionUndoEvent::SurfaceSectionUndoEvent(
	bool doadd, ObjectID oid,
        const SectionID& sectionid, const char* nm)
    : object_( oid )
    , sid_( sectionid )
    , name_( nm )
    , add_( doadd )
{}


const char* SurfaceSectionUndoEvent::getStandardDesc() const
{
    return "Modified surface section";
}


bool SurfaceSectionUndoEvent::unDo()
{
    return action( !add_ );
}


bool SurfaceSectionUndoEvent::reDo()
{
    return action( add_ );
}


const char* SurfaceSectionUndoEvent::addKey() { return "Add"; }
const char* SurfaceSectionUndoEvent::objKey() { return "Object"; }
const char* SurfaceSectionUndoEvent::sectionKey() { return "Section"; }
const char* SurfaceSectionUndoEvent::nameKey() { return "Name"; }


void SurfaceSectionUndoEvent::fillPar( IOPar& iopar ) const
{
    iopar.setYN( addKey(), add_ );
    iopar.set( objKey(), object_ );
    iopar.set( sectionKey(), (int) sid_ );
    if ( add_ ) iopar.set( nameKey(), name_ );
}


bool SurfaceSectionUndoEvent::usePar( const IOPar& iopar )
{
    int tmpsection;
    bool res = iopar.getYN( addKey(), add_ ) && iopar.get( objKey(), object_ )
	    && iopar.get( sectionKey(), tmpsection );
    if ( res )
    {
	if ( add_ )
	    res = iopar.get( nameKey(), name_ );

	if ( res )
	    sid_ = tmpsection;
    }

    return res;
}


bool SurfaceSectionUndoEvent::action( bool doadd ) const
{
    EMManager& manager = EMM();
    EMObject* objectptr = manager.getObject(object_);
    Surface* emsurface = dynamic_cast<Surface*>(objectptr);

    if ( doadd )
	return emsurface->geometry().addSection( name_.buf(), sid_, false );

    emsurface->geometry().removeSection( sid_, false );
    return true;
}


// ***** SurfaceGeometry *****

SurfaceGeometry::SurfaceGeometry( Surface& surf_ )
    : changed_( false )
    , surface_( surf_ )
{
    surface_.change.notify( mCB(this,SurfaceGeometry,geomChangeCB) );
}


SurfaceGeometry::~SurfaceGeometry()
{
    surface_.change.remove( mCB(this,SurfaceGeometry,geomChangeCB) );
    removeAll();
}


void SurfaceGeometry::removeAll()
{
    while ( nrSections() )
	removeSection( sectionID(0), false );
}


void SurfaceGeometry::geomChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EMObjectCallbackData&,cbdata,cb);
    if ( cbdata.event == EMObjectCallbackData::PositionChange )
       changed_ = true;	
}


bool SurfaceGeometry::enableChecks( bool yn )		{ return false; }
bool SurfaceGeometry::isChecksEnabled() const		{ return false; }
bool SurfaceGeometry::isNodeOK( const PosID& ) const	{ return true; }


/*
void SurfaceGeometry::removeEmptySections()
{
    const int nrsections = nrSections();
    for ( int sidx=0; sidx<nrsections; sidx++ )
    {
	const SectionID sid = sectionID( sidx );
	const Geometry::ParametricSurface* psurf = sectionGeometry( sid );
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
*/


int SurfaceGeometry::nrSections() const
{
    return sids_.size();
}


SectionID SurfaceGeometry::sectionID( int idx ) const
{
    return sids_[idx];
}


SectionID SurfaceGeometry::sectionID( const char* nm ) const
{
    for ( int idx=0; idx<sectionnames_.size(); idx++ )
	if ( sectionnames_.get(idx) == nm ) return sids_[idx];
    return -1;
}


const char* SurfaceGeometry::sectionName( const SectionID& sid ) const
{
    int idx = sids_.indexOf(sid);
    const char* res = idx!=-1 ? sectionnames_[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


bool SurfaceGeometry::setSectionName( const SectionID& sid, const char* nm,
				      bool addtoundo )
{
    const int idx = sids_.indexOf(sid);
    if ( !nm || !*nm || idx<0 )
	return false;

    sectionnames_.get(idx) = nm;

    if ( addtoundo )
	pErrMsg("Section namechange undo not implemented" );

    changed_ = true;
    return true;
}


bool SurfaceGeometry::hasSection( const SectionID& sid ) const
{ return sectionIndex(sid)!=-1; }

int SurfaceGeometry::sectionIndex( const SectionID& sid ) const
{ return sids_.indexOf(sid); }


SectionID SurfaceGeometry::addSection( const char* nm, bool addtoundo )
{
    SectionID res = 0;
    while ( sids_.indexOf(res)!=-1 ) res++;

    return addSection( nm, res, addtoundo );
}


SectionID SurfaceGeometry::addSection( const char* nm, const SectionID& sid, 
				       bool addtoundo )
{
    Geometry::Element* newsurf = createSectionGeometry();
    return newsurf ? addSectionInternal( newsurf, nm, sid, addtoundo ) : -1;
}


bool SurfaceGeometry::removeSection( const SectionID& sid, bool addtoundo )
{
    int idx=sids_.indexOf(sid);
    if ( idx==-1 ) return false;

    for ( int attr=0; attr<surface_.nrPosAttribs(); attr++ )
    {
	const TypeSet<PosID>* attrset = surface_.getPosAttribList( attr );
	if ( !attrset ) continue;
	for ( int idy=0; idy<attrset->size(); idy++ )
	{
	    const PosID& posid = (*attrset)[idy];
	    if ( sid == posid.sectionID() )
		surface_.setPosAttrib( posid, attr, false, addtoundo );
	}
    }

    //Keep the section in mem until everyone is notified
    PtrMan<const Geometry::Element> removedelem = sections_[idx];
    sections_.removeSingle( idx );
    sids_.removeSingle( idx );
    sectionnames_.removeSingle( idx );

    if ( addtoundo )
    {
	pErrMsg("Undo not implemented for remove section");
	EMM().undo().removeAllBeforeCurrentEvent();
	/*

	BufferString name = *sectionnames_[idx];
	UndoEvent* undo =
	    new SurfaceSectionUndoEvent( false, surface_.id(),
					    sid, name );
	EMM().undo().addEvent( undo, 0, 0 );
	*/
    }

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface_.id(), sid, 0 );
    surface_.change.enable( true );
    surface_.change.trigger( cbdata );

    changed_ = true;
    return true;
}


SectionID SurfaceGeometry::cloneSection( const SectionID& sid )
{
    int sectionidx = sids_.indexOf(sid);
    if ( sectionidx==-1 ) return -1;

    Geometry::Element* newsurf = sections_[sectionidx]->clone();
    const SectionID res = addSectionInternal( newsurf, 0, -1, true );

    return res;
}


const Geometry::Element*
    SurfaceGeometry::sectionGeometry( const SectionID& sid ) const
{
    const int idx = sids_.indexOf( sid );
    return idx==-1 ? 0 : sections_[idx];
}


Geometry::Element*
    SurfaceGeometry::sectionGeometry( const SectionID& sid )
{
    const int idx = sids_.indexOf( sid );
    return idx==-1 ? 0 : sections_[idx];
}


bool SurfaceGeometry::isAtEdge(EM::PosID const&) const
{ return false; }


int SurfaceGeometry::getConnectedPos( const PosID& posid,
				      TypeSet<PosID>* res) const
{
    return 0;
}


/*
bool SurfaceGeometry::findClosestNodes( TopList<float,PosID>& toplist,
				    const Coord3& pos) const
{
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
	findClosestNodes( sectionID(idx), toplist, pos, t2dfunc );

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
		Coord3 pos = surface_.getPos( sid, rc.toInt64() );
		if ( t2dfunc )
		    pos.z = t2dfunc->getValue( pos.z );

		double dist = pos.distance( origpos );
		toplist.addValue( dist,
			      PosID(surface_.id(),sid,rc.toInt64()) );
	    
	    }
	}
    }

    return toplist.size();
}


bool SurfaceGeometry::findClosestMesh( PosID& res, const Coord3& timepos,
			           const FloatMathFunction* t2dfunc ) const
{
    TopList<float, PosID> closestnodes( 20, mUdf(float), false );
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

	    const StepInterval<int> colrange = colRange(sid,-1);

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(sid,idx) )
			nodes += PosID(surface_.id(),sid,idx.toInt64());
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
    const Coord3 tpos = surface_.getPos(getNeighbor(node,dirs[nodeindex])); \
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
    const Coord3 nodetpos = surface_.getPos(node);
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
	return mUdf(float);

    Coord3 meshnormal;
    if ( !computeMeshNormal(meshnormal,closestmesh,t2dfunc) )
	return mUdf(float);

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
    const SectionID sid = nodealias.sectionID(); \
    const RowCol noderc(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    coordname = surface_.getPos(sid, neighborrc.toInt64()); \
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

    c00 = surface_.getPos(pid);
    c00def = c00.isDefined();
    if ( c00def && t2dfunc ) c00.z = t2dfunc->getValue(c00.z);

    mGetNeigborCoord( c10, c10def, +step_.row, +0 );
    mGetNeigborCoord( c01, c01def, +0, +step_.col );
    mGetNeigborCoord( c11, c11def, +step_.row, +step_.col );
}

*/


int SurfaceGeometry::findPos( const SectionID& sid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<PosID>* res ) const	
{
    return 0;


    /* TODO: Move this to some inheriting class

    int idx = sids_.indexOf(sid);
    if ( idx<0 ) return 0;

    TypeSet<PosID> posids;
    TypeSet<GeomPosID> nodes;
    sections_[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width()/2, y.width()/2, z.width()/2, nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const SectionID sid = sids_[idx];
	const PosID posid( surface_.id(), sid, 
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
}


int SurfaceGeometry::findPos( const Interval<float>& x,
			      const Interval<float>& y,
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
    Interval<float> xinterval( (float) xypos.x, (float) xypos.x );
    Interval<float> yinterval( (float) xypos.y, (float) xypos.y );

    xypos = SI().transform(cs.hrg.stop);
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    xypos = SI().transform( BinID(cs.hrg.start.inl,cs.hrg.stop.crl) );
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    xypos = SI().transform( BinID(cs.hrg.stop.inl,cs.hrg.start.crl) );
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    TypeSet<PosID> posids;
    findPos( xinterval, yinterval, cs.zrg, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(surface_.getPos(posid));

	if ( nodebid.inl<cs.hrg.start.inl || nodebid.inl>cs.hrg.stop.inl ||
	     nodebid.crl<cs.hrg.start.crl || nodebid.crl>cs.hrg.stop.crl )
	{
	    posids.removeSingle( idx--, false );
	    continue;
	}
    }

    if ( res ) res->append(posids);
    return posids.size();
}


void SurfaceGeometry::getLinkedPos( const PosID& posid,
				TypeSet<PosID>& res ) const
{ res.erase(); }


bool SurfaceGeometry::isLoaded() const
{
    return nrSections();
}


bool SurfaceGeometry::isFullResolution() const
{
    return true;
}


Executor* SurfaceGeometry::loader( const SurfaceIODataSelection* newsel )
{
    PtrMan<IOObj> ioobj = IOM().get( surface_.multiID() );
    if ( !ioobj )
	{ surface_.errmsg_ = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	{ surface_.errmsg_ = tr ? tr->errMsg() :
	    "Cannot find Translator"; return 0; }

    SurfaceIODataSelection& sel = tr->selections();
    if ( newsel && !sel.rg.isEmpty() )
    {
	sel.sellinenames = newsel->sellinenames;
	sel.seltrcranges = newsel->seltrcranges;
	
	sel.rg.start.inl = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().start ) );
	sel.rg.start.crl = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().start ) );
	sel.rg.stop.inl = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().stop ) );
	sel.rg.stop.crl = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().stop ) );
	int stepfactorinl = mNINT32(((float)newsel->rg.step.inl/sel.rg.step.inl));
	if ( stepfactorinl<1 ) stepfactorinl = 1;
	sel.rg.step.inl *= stepfactorinl;
	int stepfactorcrl = mNINT32(((float)newsel->rg.step.crl/sel.rg.step.crl));
	if ( stepfactorcrl<1 ) stepfactorcrl = 1;
	sel.rg.step.crl *= stepfactorcrl;

	sel.selvalues = newsel->selvalues;
	if ( !newsel->selsections.isEmpty() )
	    sel.selsections = newsel->selsections;
    }
    else
	sel.selvalues.erase();

    Executor* exec = tr->reader( surface_ );
    surface_.errmsg_ = tr->errMsg();
    return exec;
}


Executor* SurfaceGeometry::saver( const SurfaceIODataSelection* newsel,
       			          const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : surface_.multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	{ surface_.errmsg_ = "Cannot find surface"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !tr || !tr->startWrite(surface_) )
	{ surface_.errmsg_ = tr ? tr->errMsg() : "No Translator"; return 0; }

    SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }

    Executor* exec = tr->writer( *ioobj, changed_ );
    surface_.errmsg_ = tr->errMsg();
    return exec;
}


bool SurfaceGeometry::usePar( const IOPar& par )
{ return true; }


void SurfaceGeometry::fillPar( IOPar& par ) const
{ }


EMObjectIterator* SurfaceGeometry::createIterator( const SectionID&,
       						   const CubeSampling* ) const
{ return 0; }


SectionID SurfaceGeometry::addSectionInternal( Geometry::Element* surf,
				   const char* nm, const SectionID& wantedsid,
				   bool addtoundo )
{
    if ( !surf ) return -1;

    SectionID sid = wantedsid;
    if ( sid==-1 )
    {
	sid = 0;
	while ( sids_.indexOf(sid)!=-1 ) sid++;
    }
    
    BufferString name; 
    if ( nm && *nm ) name = nm;
    else { name = "["; name += sid + 1; name += "]"; }

    sids_ += sid;
    sectionnames_.add( name );
    sections_ += surf;

    if ( addtoundo )
    {
	pErrMsg("Undo not implemented for add section");
	EMM().undo().removeAllBeforeCurrentEvent();
    }

    enableChecks( isChecksEnabled() ); 

    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::SectionChange;
    cbdata.pid0 = PosID( surface_.id(), sid, 0 );
    surface_.change.enable( true );
    surface_.change.trigger(cbdata);

    changed_ = true;
    return sid;
}


RowColSurfaceGeometry::RowColSurfaceGeometry( Surface& s )
    : SurfaceGeometry( s )
{}


RowColSurfaceGeometry::~RowColSurfaceGeometry()
{}


Geometry::RowColSurface*
RowColSurfaceGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return reinterpret_cast<Geometry::RowColSurface*>( res );
}

const Geometry::RowColSurface*
RowColSurfaceGeometry::sectionGeometry( const SectionID& sid ) const
{
    return const_cast<RowColSurfaceGeometry*>(this)->sectionGeometry( sid );
}


StepInterval<int> RowColSurfaceGeometry::rowRange( const SectionID& sid ) const
{
    if ( sid == -1 )
	return rowRange();

    const Geometry::RowColSurface* elem = sectionGeometry( sid );
    return elem->rowRange();
}


StepInterval<int> RowColSurfaceGeometry::rowRange() const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	Geometry::RowColSurface* surf =
	    (Geometry::RowColSurface*) sections_[idx];
	const StepInterval<int> sectionrg = surf->rowRange();
	if ( sectionrg.start>sectionrg.stop )
	    continue;

	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


StepInterval<int> RowColSurfaceGeometry::colRange( const SectionID& sid,
       						   int row ) const
{
    if ( sid == -1 )
	return colRange();

    const Geometry::RowColSurface* elem = sectionGeometry( sid );
    return row<0 ? elem->colRange() : elem->colRange( row );
}


StepInterval<int> RowColSurfaceGeometry::colRange() const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	Geometry::RowColSurface* surf =
	    		(Geometry::RowColSurface*) sections_[idx];

	StepInterval<int> sectionrg = surf->colRange();
	if ( sectionrg.start>sectionrg.stop )
	    continue;

	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


StepInterval<int> RowColSurfaceGeometry::colRange( int row ) const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	Geometry::RowColSurface* surf =
	    (Geometry::RowColSurface*) sections_[idx];
	StepInterval<int> sectionrg = surf->colRange( row );
	if ( sectionrg.start>sectionrg.stop )
	    continue;

	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


EMObjectIterator* RowColSurfaceGeometry::createIterator( 
			const SectionID& sid, const CubeSampling* cs ) const
{
    return new RowColIterator( surface_, sid, cs );
}

}; //namespace

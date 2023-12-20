/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurfacegeometry.h"

#include "emrowcoliterator.h"
#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "undo.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "uistrings.h"

namespace EM {

// ***** SurfaceGeometry *****

SurfaceGeometry::SurfaceGeometry( Surface& surf )
    : changed_( false )
    , surface_( surf )
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
	const Geometry::ParametricSurface* psurf = geometryElement();
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
    return 1;
}


SectionID SurfaceGeometry::sectionID( int idx ) const
{
    return SectionID::def();
}


SectionID SurfaceGeometry::sectionID( const char* nm ) const
{
    return SectionID::def();
}


const char* SurfaceGeometry::sectionName( const SectionID& sid ) const
{
    return "Section";
}


bool SurfaceGeometry::setSectionName( const SectionID& sid, const char* nm,
				      bool addtoundo )
{
    return true;
}


bool SurfaceGeometry::hasSection( const SectionID& sid ) const
{ return true; }

int SurfaceGeometry::sectionIndex( const SectionID& sid ) const
{ return 0; }


SectionID SurfaceGeometry::addSection( const char* nm, bool addtoundo )
{
    return SectionID::udf();
}


SectionID SurfaceGeometry::addSection( const char* nm, const SectionID& sid,
				       bool addtoundo )
{
    return SectionID::udf();
}


bool SurfaceGeometry::removeSection( const SectionID& sid, bool addtoundo )
{
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
    deepErase( sections_ );

    if ( addtoundo )
    {
	EMM().undo(surface_.id()).removeAllBeforeCurrentEvent();
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
    return SectionID::udf();
}


Geometry::Element* SurfaceGeometry::geometryElement()
{
    if ( sections_.isEmpty() )
	sections_ += createGeometryElement();

    return sections_.first();
}


const Geometry::Element* SurfaceGeometry::geometryElement() const
{
    return cCast(SurfaceGeometry*,this)->geometryElement();
}


const Geometry::Element*
	SurfaceGeometry::sectionGeometry( const SectionID& ) const
{
    return geometryElement();
}


Geometry::Element* SurfaceGeometry::sectionGeometry( const SectionID& )
{
    return geometryElement();
}


bool SurfaceGeometry::isAtEdge( EM::PosID const& ) const
{ return false; }


int SurfaceGeometry::getConnectedPos( const PosID&, TypeSet<PosID>* ) const
{
    return 0;
}


/*
bool SurfaceGeometry::findClosestNodes( TopList<float,PosID>& toplist,
				    const Coord3& pos) const
{
    findClosestNodes( toplist, pos, t2dfunc );
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
    for ( rc.row()=rowrange.start;rc.row()<=rowrange.stop;
	  rc.row()+=rowrange.step)
    {
	for ( rc.col()=colrange.start; rc.col()<=colrange.stop;
						    rc.col()+=colrange.step )
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


bool SurfaceGeometry::computeNormal( Coord3& res, const TrcKeyZSampling* cs,
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
	    for ( ; rowrange.includes( idx.row() ); idx.row()+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col() );
		      idx.col()+=colrange.step )
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
    const RowCol noderc(nodealias.subID()); \
    const RowCol neighborrc( noderc.row() rowdiff, noderc.col() coldiff ); \
    coordname = surface_.getPos( neighborrc.toInt64()); \
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

    mGetNeigborCoord( c10, c10def, +step_.row(), +0 );
    mGetNeigborCoord( c01, c01def, +0, +step_.col() );
    mGetNeigborCoord( c11, c11def, +step_.row(), +step_.col() );
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
	sum += findPos( x, y, z, res );

    return sum;
}


int SurfaceGeometry::findPos( const TrcKeyZSampling& cs,
			  TypeSet<PosID>* res ) const
{
    Coord xypos = SI().transform(cs.hsamp_.start_);
    Interval<float> xinterval( (float) xypos.x, (float) xypos.x );
    Interval<float> yinterval( (float) xypos.y, (float) xypos.y );

    xypos = SI().transform(cs.hsamp_.stop_);
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    xypos = SI().transform(
	BinID(cs.hsamp_.start_.inl(),cs.hsamp_.stop_.crl()) );
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    xypos = SI().transform(
	BinID(cs.hsamp_.stop_.inl(),cs.hsamp_.start_.crl()) );
    xinterval.include( (float) xypos.x );
    yinterval.include( (float) xypos.y );

    TypeSet<PosID> posids;
    findPos( xinterval, yinterval, cs.zsamp_, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const PosID& posid = posids[idx];
	const BinID nodebid = SI().transform(surface_.getPos(posid));

	if ( nodebid.inl()<cs.hsamp_.start_.inl() ||
	     nodebid.inl()>cs.hsamp_.stop_.inl() ||
	     nodebid.crl()<cs.hsamp_.start_.crl() ||
	     nodebid.crl()>cs.hsamp_.stop_.crl() )
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
    {
	surface_.errmsg_ = uiStrings::sCantFindSurf();
	return nullptr;
    }

    PtrMan<EMSurfaceTranslator> trans =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !trans || !trans->startRead(*ioobj) )
    {
	surface_.errmsg_ = trans ? trans->errMsg() :
					    tr("Cannot find Translator");
	return nullptr;
    }

    SurfaceIODataSelection& sel = trans->selections();
    if ( newsel && !sel.rg.isEmpty() )
    {
	sel.sellinenames = newsel->sellinenames;
	sel.seltrcranges = newsel->seltrcranges;

	sel.rg.start_.inl() = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().start ) );
	sel.rg.start_.crl() = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().start ) );
	sel.rg.stop_.inl() = sel.rg.inlRange().limitValue(
		sel.rg.inlRange().snap( newsel->rg.inlRange().stop ) );
	sel.rg.stop_.crl() = sel.rg.crlRange().limitValue(
		sel.rg.crlRange().snap( newsel->rg.crlRange().stop ) );
	int stepfactorinl = mNINT32(((float)newsel->rg.step_.inl()
			  / sel.rg.step_.inl()));
	if ( stepfactorinl<1 ) stepfactorinl = 1;
	sel.rg.step_.inl() *= stepfactorinl;
	int stepfactorcrl = mNINT32(((float)newsel->rg.step_.crl()
			  / sel.rg.step_.crl()));
	if ( stepfactorcrl<1 ) stepfactorcrl = 1;
	sel.rg.step_.crl() *= stepfactorcrl;

	sel.selvalues = newsel->selvalues;
	if ( !newsel->selsections.isEmpty() )
	    sel.selsections = newsel->selsections;
    }
    else
	sel.selvalues.erase();

    Executor* exec = trans->reader( surface_ );
    surface_.errmsg_ = trans->errMsg();
    return exec;
}


Executor* SurfaceGeometry::saver( const SurfaceIODataSelection* newsel,
			          const MultiID* key )
{
    const MultiID& mid = key && !key->isUdf() ? *key : surface_.multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	surface_.errmsg_ = uiStrings::sCantFindSurf();
	return nullptr;
    }

    PtrMan<EMSurfaceTranslator> trans =
			(EMSurfaceTranslator*)ioobj->createTranslator();
    if ( !trans )
    {
	surface_.errmsg_ = ::toUiString("Internal: No Translator");
	return nullptr;
    }

    if ( !trans->startWrite(surface_) )
    {
	surface_.errmsg_ = trans->errMsg();
	return nullptr;
    }


    SurfaceIODataSelection& sel = trans->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }

    Executor* exec = trans->writer( *ioobj, changed_ );
    surface_.errmsg_ = trans->errMsg();
    return exec;
}


bool SurfaceGeometry::usePar( const IOPar& )
{ return true; }


void SurfaceGeometry::fillPar( IOPar& ) const
{}


SectionID SurfaceGeometry::addSectionInternal( Geometry::Element* surf,
				   const char* nm, const SectionID& wantedsid,
				   bool addtoundo )
{
    return SectionID::udf();
}


RowColSurfaceGeometry::RowColSurfaceGeometry( Surface& s )
    : SurfaceGeometry( s )
{}


RowColSurfaceGeometry::~RowColSurfaceGeometry()
{}


Geometry::RowColSurface* RowColSurfaceGeometry::geometryElement()
{
    return sCast(Geometry::RowColSurface*,SurfaceGeometry::geometryElement());
}

const Geometry::RowColSurface* RowColSurfaceGeometry::geometryElement() const
{
    return sCast(const Geometry::RowColSurface*,
		 SurfaceGeometry::geometryElement());
}


StepInterval<int> RowColSurfaceGeometry::rowRange( const SectionID& ) const
{
    const Geometry::RowColSurface* elem = geometryElement();
    return elem ? elem->rowRange() : StepInterval<int>::udf();
}


StepInterval<int> RowColSurfaceGeometry::rowRange() const
{
    auto* surf = geometryElement();
    if ( !surf )
	return StepInterval<int>::udf();

    const StepInterval<int> sectionrg = surf->rowRange();
    if ( sectionrg.start>sectionrg.stop )
	return StepInterval<int>::udf();

    return sectionrg;
}


StepInterval<int> RowColSurfaceGeometry::colRange() const
{
    auto* surf = geometryElement();
    if ( !surf )
	return StepInterval<int>::udf();

    StepInterval<int> sectionrg = surf->colRange();
    if ( sectionrg.start>sectionrg.stop )
	return StepInterval<int>::udf();

    return sectionrg;
}


StepInterval<int> RowColSurfaceGeometry::colRange( int row ) const
{
    const Geometry::RowColSurface* elem = geometryElement();
    return row<0 ? elem->colRange() : elem->colRange( row );
}


EMObjectIterator* RowColSurfaceGeometry::createIterator(
			const TrcKeyZSampling* tkzs ) const
{
    return new RowColIterator( surface_, tkzs );
}

} // namespace EM

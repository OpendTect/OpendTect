/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurface.cc,v 1.59 2004-07-23 12:54:49 kristofer Exp $";

#include "emsurface.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "emsurfacerelations.h"

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "emhingeline.h"
#include "emhistoryimpl.h"
#include "emmanager.h"
#include "geomgridsurface.h"
#include "geommeshsurface.h"
#include "grid.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "linsolv.h"
#include "pca.h"
#include "toplist.h"
#include "ptrman.h"
#include "survinfo.h"
#include "settings.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


void EM::SurfaceIOData::clear()
{
    dbinfo = "";
    deepErase(valnames);
    deepErase(sections);
}

void EM::SurfaceIOData::use( const EM::Surface& surf )
{
    clear();

    StepInterval<int> hrg;
    surf.getRange( hrg, true );
    rg.start.inl = hrg.start; rg.stop.inl = hrg.stop;
    rg.step.inl = hrg.step;
    surf.getRange( hrg, false );
    rg.start.crl = hrg.start; rg.stop.crl = hrg.stop;
    rg.step.crl = hrg.step;

    for ( int idx=0; idx<surf.nrSections(); idx++ )
	sections += new BufferString( surf.sectionName( surf.sectionID(idx) ) );

    for ( int idx=0; idx<surf.nrAuxData(); idx++ )
	valnames += new BufferString( surf.auxDataName(idx) );
}


void EM::SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar sectionpar;
    sections.fillPar( sectionpar );
    iopar.mergeComp( sectionpar, sSections );
}


void EM::SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* sectionpar = iopar.subselect(sSections);
    if ( sectionpar ) sections.usePar( *sectionpar );
}


void EM::SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    selvalues.erase(); selsections.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


EM::Surface::Surface( EMManager& man, const EM::ObjectID& id_ )
    : EMObject( man, id_ )
    , step_( SI().getStep(true,false), SI().getStep(false,false) )
    , loadedstep( SI().getStep(true,false), SI().getStep(false,false) )
    , rowinterval(0)
    , colinterval(0)
    , sectionchnotifier( this )
    , shift(0)
    , changed( 0 )
    , relations( *new SurfaceRelations(*this ) )
    , edgelinesets( *new EdgeLineManager(*this) )
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
}


EM::Surface::~Surface()
{
    cleanUp();
    delete &relations;
    delete &edgelinesets;
}


void EM::Surface::cleanUp()
{
    while ( nrSections() ) removeSection(sectionID(0), false);

    delete rowinterval;
    delete colinterval;
    rowinterval = 0;
    colinterval = 0;

    relations.removeAll();
    edgelinesets.removeAll();
}


void EM::Surface::removeAuxData()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepEraseArr( *auxdata[idx] );
	delete auxdata[idx];
	auxdata.replace( 0, idx );
    }

    deepErase( auxdata );
}


bool EM::Surface::findClosestNodes(TopList<float,EM::PosID>& toplist,
				const Coord3& pos_,
				const MathFunction<float>* time2depthfunc) const
{
    const int nrsections = nrSections();
    for ( int section=0; section<nrsections; section++ )
	findClosestNodes( sectionID(section), toplist, pos_, time2depthfunc );

    return toplist.size();
}


bool EM::Surface::findClosestNodes(const SectionID& sectionid,
				TopList<float,EM::PosID>& toplist,
				const Coord3& pos_,
				const MathFunction<float>* time2depthfunc) const
{
    toplist.setTop(false);

    //TODO Make faster impl
    Coord3 origpos = pos_;
    if ( time2depthfunc ) origpos.z = time2depthfunc->getValue( pos_.z );
    const int nrsections = nrSections();

    StepInterval<int> rowrange; StepInterval<int> colrange;
    getRange( rowrange, true ); getRange( colrange, false );

    RowCol rc;
    for ( rc.row=rowrange.start;rc.row<=rowrange.stop;rc.row+=rowrange.step)
    {
	for ( rc.col=colrange.start; rc.col<=colrange.stop;
						    rc.col+=colrange.step )
	{
	    if ( isDefined(sectionid,rc) )
	    {
		Coord3 pos = getPos( sectionid, rc );
		if ( time2depthfunc )
		    pos.z = time2depthfunc->getValue( pos.z );

		double dist = pos.distance( origpos );
		toplist.addValue( dist,
				  EM::PosID(id_,sectionid,rowCol2SubID(rc)));
	    
	    }
	}
    }

    return toplist.size();
}


bool EM::Surface::findClosestMesh(EM::PosID& res, const Coord3& timepos,
			  const MathFunction<float>* time2depthfunc) const
{
    TopList<float, EM::PosID> closestnodes( 20, mUndefValue, false );
    if ( !findClosestNodes(closestnodes,timepos,time2depthfunc) )
	return false;

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    float mindist;
    bool isresset = false;
    const int nrnodes = closestnodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	EM::PosID pid = closestnodes.getAssociatedValue(idx);
	Coord3 c00, c10, c01, c11;
	bool c00def, c10def, c01def, c11def;
	getMeshCoords( pid, c00, c10, c01, c11,
			c00def, c10def, c01def, c11def, time2depthfunc );

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


bool EM::Surface::computeMeshNormal( Coord3& res, const EM::PosID& pid,
			     const MathFunction<float>* time2depthfunc ) const
{
    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( pid, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   time2depthfunc );

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


bool EM::Surface::computeNormal( Coord3& res, const CubeSampling* cs,
			    const MathFunction<float>* time2depthfunc ) const 
{
    TypeSet<EM::PosID> nodes;
    if ( cs ) findPos(*cs, &nodes );
    else
    {
	for ( int idy=0; idy<nrSections(); idy++ )
	{
	    const EM::SectionID sectionid = sectionID(idy);
	    const int nrsections = nrSections();

	    StepInterval<int> rowrange; getRange( sectionid, rowrange, true );
	    StepInterval<int> colrange; getRange( sectionid, colrange, false );

	    RowCol idx( rowrange.start, colrange.start );
	    for ( ; rowrange.includes( idx.row ); idx.row+=rowrange.step )
	    {
		for ( ; colrange.includes( idx.col ); idx.col+=colrange.step )
		{
		    if ( isDefined(sectionid,idx) )
		    {
			nodes += EM::PosID(id(),sectionid,rowCol2SubID(idx));
		    }
		}
	    }
	}
    }

    return computeNormal( res, nodes, time2depthfunc );
}


#define mComputeNormalFetchNode(nodeindex) \
if ( !fetched[nodeindex] ) \
{ \
    fetched[nodeindex]=true; \
    const Coord3 tpos = getPos(getNeighbor(node,dirs[nodeindex])); \
    if ( tpos.isDefined() )  \
    { \
	while ( coords.size()<=nodeindex ) \
	    coords += Coord3(mUndefValue,mUndefValue,mUndefValue); \
	coords[nodeindex] = Coord3(tpos,t2d ? t2d->getValue(tpos.z) : tpos.z); \
	defnodes[nodeindex] = true; \
    } \
}

bool EM::Surface::computeNormal( Coord3& res, const EM::PosID& node,
			    const MathFunction<float>* t2d ) const
{
    const Coord3 nodetpos = getPos(node);
    const bool defnode = nodetpos.isDefined();
    const Coord3 nodecoord(nodetpos,
	     		 t2d&&defnode?t2d->getValue(nodetpos.z):nodetpos.z);

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
	res = rowvector.cross(colvector).normalize();
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
	    res = average.normalize();
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
	    res = average.normalize();
	    return true;
	}
    }

    return false;
}


bool EM::Surface::computeNormal( Coord3& res, const TypeSet<EM::PosID>& nodes,
			     const MathFunction<float>* time2depthfunc ) const
{
    TypeSet<Coord3> normals;
    const int nrnodes = nodes.size();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	const EM::PosID& node = nodes[idx];
	Coord3 normal;
	if ( computeNormal(normal,nodes[idx],time2depthfunc) )
	    normals += normal;
    }

    res = estimateAverageVector( normals, false, false );
    return res.isDefined();
}


float EM::Surface::normalDistance( const Coord3& timepos,
			     const MathFunction<float>* time2depthfunc,
			     Interval<float>* meshvariation ) const
{
    EM::PosID closestmesh(0,0,0);
    if ( !findClosestMesh(closestmesh,timepos,time2depthfunc) )
	return mUndefValue;

    Coord3 meshnormal;
    if ( !computeMeshNormal(meshnormal,closestmesh,time2depthfunc) )
	return mUndefValue;

    Coord3 c00, c10, c01, c11;
    bool c00def, c10def, c01def, c11def;
    getMeshCoords( closestmesh, c00, c10, c01, c11,
	    	   c00def, c10def, c01def, c11def,
		   time2depthfunc );

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

    const Coord3 pos = time2depthfunc
	? Coord3( timepos, time2depthfunc->getValue( timepos.z ) )
	: timepos;

    const Line3 line( pos, meshnormal );
    Coord3 intersection;
    plane.intersectWith( line, intersection );
    const Coord3 vector = pos-intersection;
    return meshnormal.dot( vector );
}


char EM::Surface::whichSide( const Coord3& timepos,
			     const MathFunction<float>* time2depthfunc,
			     float fuzzy ) const
{
    Interval<float> meshvariation;
    const float dist = normalDistance( timepos, time2depthfunc, &meshvariation);

    if ( dist>meshvariation.stop+fuzzy ) return 1;
    if ( dist<meshvariation.start-fuzzy ) return -1;
    return 0;
}


#define mGetNeigborCoord( coordname, defname, rowdiff, coldiff ) \
defname = false; \
for ( int idy=0; idy<nrnodealiases; idy++ ) \
{ \
    const EM::PosID& nodealias = nodealiases[idy]; \
    const EM::SectionID sectionid = nodealias.sectionID(); \
    const RowCol noderc = subID2RowCol(nodealias.subID()); \
    const RowCol neighborrc( noderc.row rowdiff, noderc.col coldiff ); \
    coordname = getPos(sectionid, neighborrc); \
    defname = coordname.isDefined(); \
    if ( defname ) \
    { \
	if ( time2depthfunc ) \
	    coordname.z = time2depthfunc->getValue(coordname.z); \
	break; \
    } \
} \



void EM::Surface::getMeshCoords( const EM::PosID& pid,
	Coord3& c00, Coord3& c10, Coord3& c01, Coord3& c11,
	bool& c00def, bool& c10def, bool& c01def, bool& c11def,
	const MathFunction<float>* time2depthfunc ) const
{
    TypeSet<EM::PosID> nodealiases;
    getLinkedPos( pid, nodealiases );
    nodealiases += pid;
    const int nrnodealiases = nodealiases.size();

    c00 = getPos(pid);
    c00def = c00.isDefined();
    if ( c00def && time2depthfunc ) c00.z = time2depthfunc->getValue(c00.z);

    mGetNeigborCoord( c10, c10def, +step_.row, +0 );
    mGetNeigborCoord( c01, c01def, +0, +step_.col );
    mGetNeigborCoord( c11, c11def, +step_.row, +step_.col );
}


int EM::Surface::nrSections() const
{
    return sectionids.size();
}


EM::SectionID EM::Surface::sectionID( int idx ) const
{
    return sectionids[idx];
}


EM::SectionID EM::Surface::sectionID( const char* nm ) const
{
    for ( int idx=0; idx<sectionnames.size(); idx++ )
	if ( *sectionnames[idx] == nm ) return sectionids[idx];
    return -1;
}


const char* EM::Surface::sectionName( const EM::SectionID& sectionid ) const
{
    int idx = sectionids.indexOf(sectionid);
    const char* res = idx!=-1 ? sectionnames[idx]->buf() : 0;
    return  res && *res ? res : 0;
}


bool EM::Surface::hasSection( const EM::SectionID& sectionid ) const
{ return sectionNr(sectionid)!=-1; }


int EM::Surface::sectionNr( const EM::SectionID& sectionid ) const
{ return sectionids.indexOf(sectionid); }



EM::SectionID EM::Surface::addSection( const char* nm, bool addtohistory )
{
    SectionID res = 0;
    while ( sectionids.indexOf(res)!=-1 ) res++;

    addSection( nm, res, addtohistory );
    return res;
}


bool EM::Surface::addSection( const char* nm, SectionID sectionid, bool addtohistory )
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
    surfaces += newsurf;

    for ( int idx=0; idx<nrAuxData(); idx++ )
    {
	if ( !auxdata[idx] )
	    continue;

	(*auxdata[idx]) += 0;
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfaceSectionEvent( true, id(),
							sectionid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    sectionchnotifier.trigger(sectionid,this);
    changed = true;
    return true;
}


void EM::Surface::removeSection( EM::SectionID sectionid, bool addtohistory )
{
    int idx=sectionids.indexOf(sectionid);
    if ( idx==-1 ) return;

    edgelinesets.removeSection( sectionid );
    relations.removeSection( sectionid );
    BufferString name = *sectionnames[idx];

    delete surfaces[idx];
    surfaces.remove( idx );
    sectionids.remove( idx );
    sectionnames.remove( idx );

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[idx];
	auxdata[idy]->replace( 0, idx );
    }

    if ( addtohistory )
    {
	HistoryEvent* history = new SurfaceSectionEvent( false, id(),
							sectionid, name );
	manager.history().addEvent( history, 0, 0 );
    }

    sectionchnotifier.trigger(sectionid,this);
    changed = true;
}


EM::SectionID EM::Surface::cloneSection( EM::SectionID sectionid )
{
    int sectionidx = sectionids.indexOf(sectionid);
    if ( sectionidx==-1 ) return -1;

    SectionID res = addSection(0, true);
    StepInterval<int> rowrange;
    StepInterval<int> colrange;
    getRange( sectionid, rowrange, true );
    if ( rowrange.width() )
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

	    const EM::PosID src(id(),sectionid,rowCol2SubID(rc));
	    const EM::PosID dst(id(),res,rowCol2SubID(rc));
	    for ( int idy=0; idy<nrPosAttribs(); idy++ )
	    {
		const int attrib = posAttrib(idy);
		if ( isPosAttrib( src, attrib ) )
		    setPosAttrib( dst, attrib, true );
	    }
	}
    }

    edgelinesets.cloneEdgeLineSet( sectionid, res );
    return res;
}


bool EM::Surface::setPos( const SectionID& section, const RowCol& surfrc,
				   const Coord3& pos, bool autoconnect,
				   bool addtohistory)
{
    RowCol geomrowcol;
    if ( !getMeshRowCol( surfrc, geomrowcol, section ) )
	return false;

    int sectionindex=sectionids.indexOf(section);
    if ( sectionindex==-1 ) return false;

    const Geometry::PosID posid = Geometry::MeshSurface::getPosID(geomrowcol);
    Geometry::MeshSurface* surface = surfaces[sectionindex];
    const Coord3 oldpos = surface->getMeshPos( geomrowcol );

    if ( addtohistory )
    {
	EM::PosID pid( id(), section, rowCol2SubID(surfrc) );
	HistoryEvent* history = new SetPosHistoryEvent( oldpos, pid );
	manager.history().addEvent( history, 0, 0 );
    }


    if ( oldpos==pos ) return true;

    changed = true;

    TypeSet<EM::PosID> nodeonothersections;
    if ( autoconnect )
	findPos( geomrowcol, nodeonothersections );

    surface->setMeshPos( geomrowcol, pos );
    surface->setFillType( geomrowcol, Geometry::MeshSurface::Filled );

    if ( !pos.isDefined() )
	surface->shrink();

    if ( autoconnect )
    {
	for ( int idx=0; idx<nodeonothersections.size(); idx++ )
	{
	    const int sectionsurfidx =
		sectionids.indexOf(nodeonothersections[idx].sectionID());
	    double otherz = surfaces[sectionsurfidx]->getMeshPos(geomrowcol).z;
	    
	    if ( mIsEqual(otherz,pos.z,mDefEps) )
	    {
		if ( !surface->isLinked(posid, surfaces[sectionsurfidx], posid ))
		{
		    surface->setLink(posid,surfaces[sectionsurfidx],posid,true);
		    // Put to history?
		}
	    }
	}
    }

    poschnotifier.trigger( EM::PosID( id(), section, rowCol2SubID(surfrc)), this);

    return true;
}


bool EM::Surface::setPos( const EM::PosID& posid, const Coord3& newpos,
			  bool addtohistory )
{
    if ( posid.objectID()!=id() ) return false;

    return setPos( posid.sectionID(), subID2RowCol(posid.subID()),
	    	   newpos, false,addtohistory);
}


Coord3 EM::Surface::getPos( const EM::PosID& posid ) const
{
    return getPos( posid.sectionID(), subID2RowCol(posid.subID()) );
}


Coord3 EM::Surface::getPos( const SectionID& section, const RowCol& rc) const
{
    const int surfidx = sectionids.indexOf( section );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, section ) )
	return Coord3( mUndefValue, mUndefValue, mUndefValue );

    return surfaces[surfidx]->getMeshPos( geomnode );
}


void EM::Surface::getPos( const RowCol& rc, TypeSet<Coord3>& crdset ) const
{
    const int nrsubsurf = nrSections();
    for ( int surfidx=0; surfidx<nrsubsurf; surfidx++ )
    {
	Coord3 crd = getPos( sectionID(surfidx), rc );
	if ( crd.isDefined() )
	    crdset += crd;
    }
}


bool EM::Surface::isDefined( const EM::PosID& posid ) const
{
    return isDefined( posid.sectionID(), subID2RowCol(posid.subID()) );
}


bool EM::Surface::isDefined( const SectionID& section, const RowCol& rc) const
{
    const int surfidx = sectionids.indexOf( section );
    RowCol geomnode;
    if ( !getMeshRowCol( rc, geomnode, section ) )
	return false;

    return surfaces[surfidx]->isDefined( geomnode );
}


int EM::Surface::findPos( const RowCol& rowcol,
				  TypeSet<PosID>& res ) const
{
    TypeSet<Coord3> respos;
    const int nrsubsurf = nrSections();
    for ( SectionID surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( !meshsurf->isDefined( rowcol ) )
	    continue;

	Coord3 pos = meshsurf->getMeshPos( rowcol );
	EM::SubID subid = rowCol2SubID( rowcol );

	for ( int idx=0; idx<res.size(); idx++ )
	{
	    if ( subid!=res[idx].subID() )
		continue;

	    if ( mIsEqual(respos[idx].z,pos.z,mDefEps) ) continue;

	    res += PosID(id(), sectionID(surface), subid );
	    respos += pos;
	}
    }

    return res.size();
}


int EM::Surface::findPos( const EM::SectionID& sectionid,
			  const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int idx = sectionids.indexOf(sectionid);
    if ( idx<0 ) return 0;

    TypeSet<EM::PosID> posids;
    TypeSet<Geometry::PosID> nodes;
    surfaces[idx]->findPos( x.center(), y.center(), z.center(),
			    x.width(), y.width(), z.width(), nodes );

    const int nrnodes = nodes.size();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const SectionID section = sectionids[idx];
	const EM::PosID posid( id(), sectionid, getSurfSubID(nodes[idy],sectionid));

	TypeSet<EM::PosID> clones;
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


int EM::Surface::findPos( const Interval<float>& x, const Interval<float>& y,
			  const Interval<float>& z,
			  TypeSet<EM::PosID>* res ) const	
{
    int sum = 0;
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
	sum += findPos( sectionID(idx), x, y, z, res );

    return sum;
}

    
int EM::Surface::findPos( const CubeSampling& cs,
			  TypeSet<EM::PosID>* res ) const
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

    TypeSet<EM::PosID> posids;
    findPos( xinterval, yinterval, cs.zrg, &posids );

    for ( int idx=0; idx<posids.size(); idx++ )
    {
	const EM::PosID& posid = posids[idx];
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


EM::PosID EM::Surface::getNeighbor( const EM::PosID& posid,
				    const RowCol& dir ) const
{
    RowCol diff;
    if ( dir.row>0 ) diff.row = step_.row;
    if ( dir.row<0 ) diff.row = -step_.row;
    if ( dir.col>0 ) diff.col = step_.col;
    if ( dir.col<0 ) diff.col = -step_.col;
    
    TypeSet<EM::PosID> aliases;
    getLinkedPos( posid, aliases );
    aliases += posid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const RowCol ownrc = subID2RowCol(aliases[idx].subID());
	const RowCol neigborrc = ownrc+diff;
	if ( isDefined(aliases[idx].sectionID(),neigborrc) )
	    return EM::PosID( id(), aliases[idx].sectionID(),
		    	      rowCol2SubID(neigborrc));
    }

    const RowCol ownrc = subID2RowCol(posid.subID());
    const RowCol neigborrc = ownrc+diff;

    return EM::PosID( id(), posid.sectionID(), rowCol2SubID(neigborrc));
}


int EM::Surface::getNeighbors( const EM::PosID& posid_, TypeSet<EM::PosID>* res,
				int maxradius, bool circle ) const
{
    ObjectSet< TypeSet<EM::PosID> > neigbors;
    const RowCol start = subID2RowCol(posid_.subID());
    neigbors += new TypeSet<EM::PosID>( 1, posid_ );

    for ( int idx=0; idx<neigbors.size(); idx++ )
    {
	for ( int idz=0; idz<neigbors[idx]->size(); idz++ )
	{
	    EM::PosID currentposid = (*neigbors[idx])[idz];
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
		   
		    const EM::PosID
			    neighborposid(currentposid.objectID(),
			    currentposid.sectionID(),
			    rowCol2SubID(neighborrowcol) );

		    bool found = false;
		    for ( int idy=0; idy<neigbors.size(); idy++ )
		    {
			const TypeSet<EM::PosID>& posids=*neigbors[idy];
			if ( posids.indexOf(neighborposid)!=-1 )
			{
			    found = true;
			    break;
			}
		    }

		    if ( found )
			continue;

		    TypeSet<EM::PosID>& posids = *new TypeSet<EM::PosID>;
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


void EM::Surface::getLinkedPos( const EM::PosID& posid,
				TypeSet<EM::PosID>& res ) const
{
    if ( posid.objectID()!=id() )
        return; //TODO: Implement handling for this case

    const EM::SubID subid = posid.subID();
    const RowCol rowcol = subID2RowCol(subid);
    const Geometry::MeshSurface* ownmeshsurf = getSurface( posid.sectionID() );
    if ( !ownmeshsurf ) return;

    const int nrsubsurf = nrSections();
    for ( int surface=0; surface<nrsubsurf; surface++ )
    {
	Geometry::MeshSurface* meshsurf = surfaces[surface];
	if ( ownmeshsurf->isLinked( subid, meshsurf, subid ) )
	{
	    res += EM::PosID( id(),sectionids[surface], subid );
	}
    }
}


bool EM::Surface::isLoaded() const
{
    return nrSections();
}


RowCol EM::Surface::loadedStep() const
{
    return loadedstep;
}


RowCol EM::Surface::step() const
{
    return step_;
}


void EM::Surface::setTranslatorData( const RowCol& step__,
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


RowCol EM::Surface::subID2RowCol( const EM::SubID& subid )
{
    return longlong2rc(subid);
}


EM::SubID EM::Surface::rowCol2SubID( const RowCol& rc )
{
    return rc2longlong(rc);
}


bool EM::Surface::isFullResolution() const
{
    return loadedstep == step_;
}


int EM::Surface::nrAuxData() const
{
    return auxdatanames.size();
}


const char* EM::Surface::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


void EM::Surface::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames[dataidx] )
	auxdatanames.replace( new BufferString(name), dataidx );
}


int EM::Surface::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
	if ( *auxdatanames[idx] == nm ) return idx;
    return -1;
}


int EM::Surface::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<nrSections(); idx++ )
	(*newauxdata) += 0;

    changed = true;
    return auxdatanames.size()-1;
}


void EM::Surface::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( 0, dataidx );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( 0, dataidx );
    changed = true;
}


void EM::Surface::removeAllAuxdata()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );
    changed = true;
}


float EM::Surface::getAuxDataVal( int dataidx, const EM::PosID& posid ) const
{
    if ( !auxdata[dataidx] ) return mUndefValue;
    const int sectionidx = sectionids.indexOf( posid.sectionID() );
    if ( sectionidx==-1 ) return mUndefValue;

    const TypeSet<float>* sectionauxdata = (*auxdata[dataidx])[sectionidx];
    if ( !sectionauxdata ) return mUndefValue;

    RowCol geomrc;
    getMeshRowCol( posid.subID(), geomrc, posid.sectionID() );
    const int subidx = surfaces[sectionidx]->indexOf( geomrc );
    if ( subidx==-1 ) return mUndefValue;
    return (*sectionauxdata)[subidx];
}


void EM::Surface::setAuxDataVal(int dataidx,const EM::PosID& posid, float val)
{
    if ( !auxdata[dataidx] ) return;

    const int sectionidx = sectionids.indexOf( posid.sectionID() );
    if ( sectionidx==-1 ) return;

    RowCol geomrc; 
    getMeshRowCol( posid.subID(), geomrc, posid.sectionID() );
    const int subidx = surfaces[sectionidx]->indexOf( geomrc );
    if ( subidx==-1 ) return;

    TypeSet<float>* sectionauxdata = (*auxdata[dataidx])[sectionidx];
    if ( !sectionauxdata )
    {
	const int sz = surfaces[sectionidx]->size();
	auxdata[dataidx]->replace( new TypeSet<float>(sz,mUndefValue),sectionidx);
	sectionauxdata = (*auxdata[dataidx])[sectionidx];
    }

    (*sectionauxdata)[subidx] = val;
    changed = true;
}


bool EM::Surface::getMeshRowCol( const EM::SubID& subid, RowCol& meshrowcol, 
				 const SectionID& sectionid ) const
{
    return getMeshRowCol( subID2RowCol(subid), meshrowcol, sectionid );
}


bool EM::Surface::getMeshRowCol( const RowCol& emrowcol, RowCol& meshrowcol,
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


EM::SubID EM::Surface::getSurfSubID( const RowCol& nodeid, 
				     const SectionID& sectionid ) const
{
    const int idx = sectionids.indexOf( sectionid );
    RowCol origo = idx<origos.size() ? origos[idx] :
	(origos.size() ? origos[0] : RowCol(0,0) );
    return rowCol2SubID( origo+nodeid*loadedstep );
}


EM::SubID EM::Surface::getSurfSubID( const Geometry::PosID& gposid,
       				     const SectionID& sectionid ) const
{
    const RowCol& nodeid = Geometry::MeshSurface::getMeshNode(gposid);
    return getSurfSubID( nodeid, sectionid );
}



const Geometry::MeshSurface* EM::Surface::getSurface( SectionID sectionid )const
{
    const int idx = sectionids.indexOf( sectionid );
    return idx==-1 ? 0 : surfaces[idx];
}


void EM::Surface::getRange( StepInterval<int>& rg, bool rowdir ) const
{
    const int nrsections = nrSections();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	const EM::SectionID sectionid = sectionID( idx );
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


void EM::Surface::getRange( const EM::SectionID& sectionid, StepInterval<int>& rg,
			    bool rowdir ) const
{
    const Geometry::MeshSurface& gsurf = *getSurface( sectionid );
    if ( rowdir )
    {
	const RowCol firstrow(gsurf.firstRow(),0);
	const RowCol lastrow(gsurf.lastRow(),0);

	rg.start = subID2RowCol( getSurfSubID(firstrow,sectionid)).row;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,sectionid)).row;
    }
    else
    {
	const Interval<int> colrg = gsurf.getColInterval();
	const RowCol firstrow(0,colrg.start);
	const RowCol lastrow(0,colrg.stop);

	rg.start = subID2RowCol( getSurfSubID(firstrow,sectionid)).col;
	rg.stop = subID2RowCol( getSurfSubID(lastrow,sectionid)).col;
    }

    rg.step = rowdir ? loadedStep().row : loadedStep().col;
}


bool EM::Surface::isAtEdge( const EM::PosID& pid ) const
{
    if ( !isDefined(pid) ) return false;

    int nrneighbors = getNeighbors(pid,0,1,false);
    if ( nrneighbors == 6 )
    {
	const int section = pid.sectionID();
	RowCol center = subID2RowCol( pid.subID() );
	return !( isDefined( section, center+step_*RowCol(0,1) ) &&
		  isDefined( section, center+step_*RowCol(1,0) ) &&
		  isDefined( section, center+step_*RowCol(0,-1) ) &&
		  isDefined( section, center+step_*RowCol(-1,0) ) );
    }

   return nrneighbors != 8;
}


Executor* EM::Surface::loader( const EM::SurfaceIODataSelection* newsel,
       			       int attridx )
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
	{ errmsg = tr ? tr->errMsg() : "Cannot find Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }
    else
	sel.selvalues.erase();

    if ( attridx < 0 )
    {
	Executor* exec = tr->reader( *this );
	errmsg = tr->errMsg();
	return exec;
    }

    StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;
    
    const char* attrnm = sel.sd.valnames[attridx]->buf();
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) return 0;
	BufferString fnm = 
	    EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
	if ( File_isEmpty(fnm) ) { gap++; continue; }
	else gap = 0;

	EM::dgbSurfDataReader* rdr = new EM::dgbSurfDataReader(fnm);
	if ( strcmp(attrnm,rdr->dataName()) )
	{ delete rdr; continue; }

	rdr->setSurface( *this );
	return rdr;
    }

    return 0;
}


Executor* EM::Surface::saver( const EM::SurfaceIODataSelection* newsel,
       			      bool auxdata, const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startWrite(*this) )
	{ errmsg = tr ? tr->errMsg() : "No Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selsections = newsel->selsections;
    }

    if ( auxdata )
    {
	StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
	if ( !conn ) return 0;

	BufferString fnm;
	int dataidx = sel.selvalues.size() ? sel.selvalues[0] : -1;
	if ( dataidx >=0 )
	{
	    fnm = EM::dgbSurfDataWriter::createHovName( conn->fileName(),
		    					dataidx );
	}
	else
	{
	    for ( int idx=0; ; idx++ )
	    {
		fnm =EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
		if ( !File_exists(fnm) )
		    break;
	    }
	}

	bool binary = true;
	mSettUse(getYN,"dTect.Surface","Binary format",binary);
	Executor* exec = new EM::dgbSurfDataWriter(*this,0,0,binary,fnm);
	return exec;
    }
    else
    {
	Executor* exec = tr->writer(*ioobj);
	errmsg = tr->errMsg();
	return exec;
    }
}


bool EM::Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && relations.usePar(par) &&
	   edgelinesets.usePar(par);
}


void EM::Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar(par);
    relations.fillPar(par);
    edgelinesets.fillPar(par);
}

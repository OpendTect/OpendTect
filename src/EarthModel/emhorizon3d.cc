/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon3d.cc,v 1.52 2005-02-10 16:22:35 nanne Exp $
________________________________________________________________________

-*/

#include "emhorizon.h"

#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "geomgridsurface.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "binidvalset.h"
#include "trigonometry.h"

#include <math.h>

namespace EM {

Horizon::Horizon( EMManager& man, const ObjectID& id_ )
    : Surface(man,id_,*new HorizonGeometry(*this))
{}


const char* Horizon::typeStr() { return EMHorizonTranslatorGroup::keyword; }

void Horizon::initClass(EMManager& emm)
{
    emm.addFactory( new ObjectFactory( create,
				       EMHorizonTranslatorGroup::ioContext(),
				       typeStr()) );
}


EMObject* Horizon::create( const ObjectID& id, EMManager& emm )
{ return new Horizon( emm, id ); }



class AuxDataImporter : public Executor
{
public:

AuxDataImporter( Horizon& hor, const ObjectSet<BinIDValueSet>& sects_ )
    : Executor("Data Import")
    , horizon(hor)
    , sections(sects_)
    , totalnr(0)
    , nrdone(0)
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections[idx];
	totalnr += bvs.nrInls();
    }

    const int nrvals = sections[0]->nrVals();
    for ( int validx=1; validx<nrvals; validx++ )
    {
	BufferString nm( "Imported attribute " ); nm += validx;
	horizon.auxdata.addAuxData( nm );
    }

    sectionidx = -1;
}


const char*	message() const { return "Importing aux data"; }
int		totalNr() const { return totalnr; }
int		nrDone() const { return nrdone; }
const char*	nrDoneText() const { return "Nr inlines imported"; }

int nextStep()
{
    if ( sectionidx == -1 || inl > inlrange.stop )
    {
	sectionidx++;
	if ( sectionidx >= horizon.geometry.nrSections() )
	    return Finished;

	SectionID sectionid = horizon.geometry.sectionID(sectionidx);
	horizon.geometry.getRange( sectionid, inlrange, true );
	horizon.geometry.getRange( sectionid, crlrange, false );
	inl = inlrange.start;
    }

    PosID posid( horizon.id(), horizon.geometry.sectionID(sectionidx) );
    const BinIDValueSet& bvs = *sections[sectionidx];
    const int nrvals = bvs.nrVals();
    for ( int crl=crlrange.start; crl<=crlrange.stop; crl+=crlrange.step )
    {
	const BinID bid(inl,crl);
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	const float* vals = bvs.getVals( pos );
	RowCol rc( HorizonGeometry::getRowCol(bid) );
	posid.setSubID( HorizonGeometry::rowCol2SubID(rc) );
	for ( int validx=1; validx<nrvals; validx++ )
	    horizon.auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
    }

    inl += inlrange.step;
    nrdone++;
    return MoreToDo;
}

protected:

    const ObjectSet<BinIDValueSet>&	sections;
    Horizon&			horizon;
    StepInterval<int>		inlrange;
    StepInterval<int>		crlrange;

    int				inl;
    int				sectionidx;

    int				totalnr;
    int				nrdone;
};


class HorizonImporter : public Executor
{
public:

HorizonImporter( Horizon& hor, const ObjectSet<BinIDValueSet>& sects_, bool fh )
    : Executor("Horizon Import")
    , horizon(hor)
    , sections(sects_)
    , fixholes(fh)
    , step(0,0)
    , totalnr(0)
    , nrdone(0)
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	const BinIDValueSet& bvs = *sections[idx];
	totalnr += bvs.nrInls();

	BinID bid00, bid11;
	bvs.get( BinIDValueSet::Pos(0,0), bid00 );
	bvs.get( BinIDValueSet::Pos(1,1), bid11 );
	RowCol step_( abs(bid00.inl-bid11.inl), abs(bid00.crl-bid11.crl) );
	if ( !step_.row ) step_.row = 1;
	if ( !step_.col ) step_.col = 1;
	if ( !step.inl && !step.crl ) step = BinID(step_.row,step_.col);

	SectionID sectionid =  hor.geometry.addSection( 0, false );
	horizon.geometry.setTranslatorData( sectionid, step_, step_, 
					    RowCol(bid00.inl,bid00.crl) );
    }

    sectionidx = -1;
}


const char*	message() const { return "Transforming grid data"; }
int		totalNr() const { return totalnr; }
int		nrDone() const { return nrdone; }
const char*	nrDoneText() const { return "Nr inlines imported"; }

int nextStep()
{
    if ( sectionidx == -1 || inl > inlrange.stop )
    {
	sectionidx++;
	if ( sectionidx >= horizon.geometry.nrSections() )
	    return fixholes ? fillHoles() : Finished;

	inlrange = sections[sectionidx]->inlRange();
	crlrange = sections[sectionidx]->crlRange();
	inl = inlrange.start;
    }

    PosID posid( horizon.id(), horizon.geometry.sectionID(sectionidx) );
    const BinIDValueSet& bvs = *sections[sectionidx];
    const int nrvals = bvs.nrVals();
    for ( int crl=crlrange.start; crl<=crlrange.stop; crl+=step.crl )
    {
	const BinID bid(inl,crl);
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	const float* vals = bvs.getVals( pos );
	Coord crd = SI().transform( bid );
	RowCol rc( HorizonGeometry::getRowCol(bid) );
	posid.setSubID( HorizonGeometry::rowCol2SubID(rc) );
	horizon.geometry.setPos( posid, Coord3(crd.x,crd.y,vals[0]), false );
    }

    inl += step.inl;
    nrdone++;
    return MoreToDo;
}


int fillHoles()
{
    bool changed = false;
    for ( int idx=0; idx<horizon.geometry.nrSections(); idx++ )
    {
	SectionID sectionid = horizon.geometry.sectionID( idx );
	StepInterval<int> rowrange;
	horizon.geometry.getRange( sectionid, rowrange, true );
	if ( rowrange.width(false)>=0 )
	{
	    StepInterval<int> colrange;
	    horizon.geometry.getRange( sectionid, colrange, false );
	    PosID pid( horizon.id(), sectionid );

	    for ( int currow=rowrange.start; rowrange.includes(currow);
		  currow+=rowrange.step )
	    {
		for ( int curcol=colrange.start; colrange.includes(curcol);
		      curcol+=colrange.step )
		{
		    const RowCol rc( currow, curcol );
		    if ( horizon.geometry.isDefined(sectionid,rc) )
			continue;

		    pid.setSubID( horizon.geometry.rowCol2SubID(rc) );
		    TypeSet<PosID> neighbors;
		    horizon.geometry.getNeighbors( pid, &neighbors );
		    if ( neighbors.size()<6 )
			continue;

		    TypeSet<Coord3> neighborcoords;
		    for ( int nidx=0; nidx<neighbors.size(); nidx++ )
			neighborcoords += horizon.getPos(neighbors[nidx]);

		    Plane3 plane;
		    if ( !plane.set(neighborcoords) )
			continue;

		    const BinID bid = HorizonGeometry::getBinID(rc);
		    const Coord coord = SI().transform(bid);
		    const Line3 line( Coord3(coord,0), Vector3(0,0,1));

		    Coord3 interpolcoord;
		    if ( !plane.intersectWith(line,interpolcoord) )
			continue;

		    horizon.setPos(pid,interpolcoord,false);
		    changed = true;
		}
	    }

	    if ( changed )
		return MoreToDo;
	}
    }

    return Finished;
}


protected:

    const ObjectSet<BinIDValueSet>&	sections;
    Horizon&			horizon;
    Interval<int>		inlrange;
    Interval<int>		crlrange;
    BinID			step;

    int				inl;
    bool			fixholes;
    int				sectionidx;

    int				totalnr;
    int				nrdone;
};


Executor* Horizon::importer( const ObjectSet<BinIDValueSet>& sections, bool fh )
{
    cleanUp();
    return new HorizonImporter( *this, sections, fh );
}


Executor* Horizon::auxDataImporter( const ObjectSet<BinIDValueSet>& sections )
{
    return new AuxDataImporter( *this, sections );
}


const IOObjContext& Horizon::getIOObjContext() const
{ return EMHorizonTranslatorGroup::ioContext(); }


HorizonGeometry::HorizonGeometry( Surface& surf )
    : SurfaceGeometry( surf ) 
{}


bool HorizonGeometry::createFromStick( const TypeSet<Coord3>& stick,
				       const SectionID& sid, float velocity )
{
    SectionID sectionid = sid;
    if ( !nrSections() || !hasSection(sid) ) 
	sectionid = addSection( "", true );

    const float idealdistance = 25; // TODO set this in some intelligent way

    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( stick[idx], stick[idx].z*velocity/2 );
	const Coord3 stoppos( stick[idx+1], stick[idx+1].z*velocity/2 );
	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	const RowCol startrc = getRowCol(startbid);
	const RowCol stoprc = getRowCol(stopbid);
	const Line3 line( Coord3(startrc.row,startrc.col,0),
	     Coord3(stoprc.row-startrc.row,stoprc.col-startrc.col,0) );

	RowCol iterstep( step_ );
	if ( startrc.row==stoprc.row ) iterstep.row = 0;
	else if ( startrc.row>stoprc.row ) iterstep.row = -iterstep.row;

	if ( startrc.col==stoprc.col ) iterstep.col = 0;
	else if ( startrc.col>stoprc.col ) iterstep.col = -iterstep.col;

	RowCol rowcol = startrc;
	while ( rowcol != stoprc )
	{
	    const float rowdistbefore = rowcol.row-startrc.row;
	    const float coldistbefore = rowcol.col-startrc.col;
	    const float distbefore = sqrt(rowdistbefore*rowdistbefore+
					  coldistbefore*coldistbefore );
	    const float rowdistafter = rowcol.row-stoprc.row;
	    const float coldistafter = rowcol.col-stoprc.col;
	    const float distafter = sqrt(rowdistafter*rowdistafter+
					  coldistafter*coldistafter );

	    const Coord3 newpos(SI().transform(getBinID(rowcol)),
		    (startpos.z*distafter+stoppos.z*distbefore)/
		    (distbefore+distafter)/(velocity/2));

	    const PosID posid( surface.id(), sectionid,
				   rowCol2SubID(rowcol) );
	    setPos( posid, newpos, true );
	    if ( rowcol == startrc )
		surface.setPosAttrib( posid, EMObject::sPermanentControlNode, true);

	    float distance = mUndefValue;
	    RowCol nextstep;
	    if ( iterstep.row )
	    {
		distance = line.distanceToPoint(
			Coord3(rowcol.row+iterstep.row, rowcol.col,0) );
		nextstep = rowcol;
		nextstep.row += iterstep.row;
	    }

	    if ( iterstep.col )
	    {
		float nd = line.distanceToPoint(
			    Coord3(rowcol.row, rowcol.col+iterstep.col,0) );
		if ( nd<distance )
		{
		    nextstep = rowcol;
		    nextstep.col += iterstep.col;	
		}
	    }

	    rowcol = nextstep;
	}

	if ( idx==stick.size()-2 )
	{
	    const PosID posid( surface.id(), sectionid,
				    rowCol2SubID(rowcol));
	    setPos( posid, Coord3( stoppos, stoppos.z/(velocity/2)), true);

	    surface.setPosAttrib( posid, EMObject::sPermanentControlNode, true);
	}
    }

    return true;
}


BinID HorizonGeometry::getBinID( const SubID& subid )
{
    return getBinID( SurfaceGeometry::subID2RowCol(subid) );
}


BinID HorizonGeometry::getBinID( const RowCol& rc )
{
    return BinID(rc.row, rc.col);
}


RowCol HorizonGeometry::getRowCol( const BinID& bid )
{
    return RowCol( bid.inl, bid.crl );
}


SubID HorizonGeometry::getSubID( const BinID& bid )
{
    return rowCol2SubID(getRowCol(bid));
}


Geometry::MeshSurface*
    HorizonGeometry::createSectionSurface( const SectionID& sectionid ) const
{
    Geometry::GridSurface* newsurf = new Geometry::GridSurface();
    setTransform( sectionid );
    return newsurf;
}


void HorizonGeometry::setTransform( const SectionID& sectionid ) const
{
    const int sectionidx = sectionNr( sectionid );
    if ( sectionidx < 0 || sectionidx >= meshsurfaces.size() )
	return;
    mDynamicCastGet(Geometry::GridSurface*,gridsurf,meshsurfaces[sectionidx])
    if ( !gridsurf ) return;

    const RowCol rc00( 0, 0 );
    const RowCol rc10( 1, 0 );
    const RowCol rc11( 1, 1 );

    const RowCol surfrc00 = subID2RowCol( getSurfSubID(rc00,sectionid) );
    const RowCol surfrc10 = subID2RowCol( getSurfSubID(rc10,sectionid) );
    const RowCol surfrc11 = subID2RowCol( getSurfSubID(rc11,sectionid) );

    const Coord pos00 = SI().transform(BinID(surfrc00.row,surfrc00.col));
    const Coord pos10 = SI().transform(BinID(surfrc10.row,surfrc10.col));
    const Coord pos11 = SI().transform(BinID(surfrc11.row,surfrc11.col));
    
    gridsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			     pos10.x, pos10.y, rc10.row, rc10.col,
			     pos11.x, pos11.y, rc11.row, rc11.col );
}


}; //namespace



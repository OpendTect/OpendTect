/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.26 2005-02-10 16:22:35 nanne Exp $
________________________________________________________________________

-*/

#include "emfault.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "errh.h"
#include "geommeshsurfaceimpl.h"
#include "survinfo.h"

namespace EM {


const char* Fault::typeStr() { return EMFaultTranslatorGroup::keyword; }


void Fault::initClass(EMManager& emm)
{
    emm.addFactory( new ObjectFactory( create,
				       EMFaultTranslatorGroup::ioContext(),
				       typeStr()) );
}


EMObject* Fault::create( const ObjectID& id, EMManager& emm )
{ return new Fault( emm, id ); }


Fault::Fault( EMManager& em_, const ObjectID& mid_ )
    : Surface(em_,mid_, *new FaultGeometry(*this) )
{}


const IOObjContext& Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


FaultGeometry::FaultGeometry(Fault& flt)
    : SurfaceGeometry( flt )
{}


FaultGeometry::~FaultGeometry()
{
}


bool FaultGeometry::isHidden( const PosID& pid ) const
{
    return surface.isPosAttrib( pid, hiddenAttrib() );
}


bool FaultGeometry::isHidden( const SectionID& sectionid,
			      const RowCol& rc ) const
{
    return isHidden( PosID( surface.id(), sectionid, rowCol2SubID(rc)) );
}


void FaultGeometry::setHidden( const EM::PosID& pid, bool yn, bool addtohistory)
{
    if ( pid.objectID()!=surface.id() )
	return;

    surface.setPosAttrib( pid, hiddenAttrib(), yn );

    if ( yn )
    {   // Will get a neighbor's pos as it's
	// hidden
	Coord3 pos = surface.getPos( pid ); 
	setPos( pid, pos, addtohistory );
    }
}


void FaultGeometry::setHidden( const EM::SectionID& sectionid,
			       const RowCol& rc, bool yn,
			       bool addtohistory )
{
    setHidden( PosID( surface.id(), sectionid, rowCol2SubID(rc)),
	       yn, addtohistory );
}



bool FaultGeometry::insertHiddenColumn( const SectionID& section, int col)
{
    if ( sectionids.size()>1 )
	pErrMsg( "TODO: Implement support for multiple sections." );

    StepInterval<int> rowrg, colrg;
    getRange( section, rowrg, true );
    if ( rowrg.width(false)<0 )
	return false;

    getRange( section, colrg, false );

    if ( col<colrg.start-1 || col>colrg.stop+1 )
	return false;

    for ( int colidx=colrg.stop; colidx>=col; colidx-=colrg.step )
    {
	for ( int rowidx=rowrg.start; rowidx<=rowrg.stop; rowidx+=rowrg.step )
	{
	    surface.changePosID(
		 PosID( surface.id(),section,
			rowCol2SubID(RowCol(rowidx,colidx))),
		 PosID( surface.id(),section,
			rowCol2SubID(RowCol(RowCol(rowidx,colidx+colrg.step)))),
		 	true );
	}
    }

    for ( int rowidx=rowrg.start; rowidx<=rowrg.stop; rowidx+=rowrg.step )
    {
	const RowCol rc(  RowCol( rowidx, col ) );
	setHidden( section, rc, true, true );
    }

    return true;
}


Coord3 FaultGeometry::getPos(const SectionID& section, const RowCol& rc) const
{
    if ( !isHidden( section, rc ) )
	return SurfaceGeometry::getPos(section,rc);

    StepInterval<int> rowrg, colrg;
    getRange( section, rowrg, true );
    if ( rowrg.width(false)>=0 )
	getRange( section, colrg, false );
    else 
	return Coord3::udf();

    int colstep = step().col;

    int stepout = colstep;
    while ( true )
    {
	const RowCol nextrc(rc.row,rc.col+stepout);
	const RowCol prevrc(rc.row,rc.col-stepout);
	if ( !colrg.includes(nextrc.col) && !colrg.includes(prevrc.col) )
	    break;

	if ( colrg.includes(nextrc.col) && !isHidden(section,nextrc)
		&& isDefined(section,nextrc) )
	    return SurfaceGeometry::getPos(section,nextrc);

	if ( colrg.includes(prevrc.col) && !isHidden(section,prevrc)
		&& isDefined(section,prevrc) )
	    return SurfaceGeometry::getPos(section,prevrc);

	stepout += colstep;
    }

    return Coord3::udf();
}


void FaultGeometry::updateHiddenPos()
{
    const TypeSet<PosID>* hiddenpids = surface.getPosAttribList(hiddenAttrib());
    if ( !hiddenpids )
	return;

    for ( int idx=0; idx<hiddenpids->size(); idx++ )
    {
	const PosID& pid = (*hiddenpids)[idx];
	setPos( pid, surface.getPos( pid ), false );
    }
}


bool FaultGeometry::createFromStick( const TypeSet<Coord3>& stick, 
				     const SectionID& sid, float velocity )
{
    if ( stick.size() < 2 ) return false;

    SectionID sectionid = sid;
    if ( !nrSections() || !hasSection(sid) ) 
	sectionid = addSection( "", true );

    setTranslatorData( sectionid, RowCol(1,1), RowCol(1,1), RowCol(0,0) );
    const float idealdistance = 25; // TODO set this in some intelligent way
    RowCol rowcol(0,0);

    bool istimestick = mIsEqual(stick[0].z,stick[stick.size()-1].z,1e-6); 

    Coord3 stoppos;
    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( SI().transform(SI().transform(stick[idx])),
				stick[idx].z*velocity/2 );
	stoppos = Coord3( SI().transform(SI().transform(stick[idx+1])),
				stick[idx+1].z*velocity/2 );

	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	TypeSet<BinID> bids;
	TypeSet<float> times;

	if ( istimestick )
	{
	    if ( startbid==stopbid )
		continue;

	    RCol::makeLine( startbid, stopbid, bids,
		    	    BinID(SI().inlStep(true),SI().crlStep(true)) );
	    bids.remove( bids.size()-1 );
	    times = TypeSet<float>( bids.size(), stick[0].z );
	}
	else
	{
	    const float distance = startpos.distance(stoppos);
	    const Coord3 vector = (stoppos-startpos).normalize();
	    const int nrofsegments = mNINT(distance/idealdistance);
	    const float segmentlength = distance/nrofsegments;

	    for ( int idy=0; idy<nrofsegments; idy++ )
	    {
		const Coord3 newrelpos( vector.x*segmentlength*idy,
					vector.y*segmentlength*idy,
					vector.z*segmentlength*idy );

		const Coord3 newprojectedpos = startpos+newrelpos;
		const BinID newprojectedbid = SI().transform( newprojectedpos );

		bids += newprojectedbid;
		times += newprojectedpos.z/(velocity/2);

	    }
	}

	for ( int idy=0; idy<bids.size(); idy++ )
	{
	    const Coord3 newpos( SI().transform(bids[idy]), times[idy] );
	    setPos( sectionid, rowcol, newpos, false, true );

	    if ( !idy )
	    {
		surface.setPosAttrib(
			PosID(surface.id(), sectionid, rowCol2SubID(rowcol)),
			EMObject::sPermanentControlNode, true);
	    }

	    istimestick ? rowcol.col++ : rowcol.row++;
	}
    }

    Coord3 crd( SI().transform(SI().transform(stick[stick.size()-1])),
	    	stick[stick.size()-1].z );
    setPos( sectionid, rowcol, crd, false, true );
    surface.setPosAttrib( PosID(surface.id(), sectionid,
			  rowCol2SubID(rowcol)),
			  EMObject::sPermanentControlNode, true);

    return true;
}


Geometry::MeshSurface*
FaultGeometry::createSectionSurface( const SectionID& pid ) const
{
    return new Geometry::MeshSurfaceImpl;
}


PosID FaultGeometry::getNeighbor( const PosID& posid,
				  const RowCol& dir ) const
{
    if ( sectionids.size()>1 )
	pErrMsg( "TODO: Implement support for multiple sections." );

    if ( !dir.col )
	return SurfaceGeometry::getNeighbor( posid, dir );

    const SectionID sectionid = posid.sectionID();

    StepInterval<int> rowrg, colrg;
    getRange( rowrg, true );
    if ( rowrg.width(false)>=0 )
	getRange( colrg, false );


    RowCol currc =  subID2RowCol( posid.subID() );
    while ( true )
    {
	currc += step_*dir;
	if ( !rowrg.includes(currc.row) || !colrg.includes(currc.col) )
	    break;

	if ( !isHidden( sectionid, currc ) )
	    break;
    }

    return PosID( surface.id(), sectionid, rowCol2SubID( currc ) );
}


int FaultGeometry::findPos( const EM::SectionID& sectionid,
			    const Interval<float>& x,
			    const Interval<float>& y,
			    const Interval<float>& z,
			    TypeSet<EM::PosID>* res_) const
{
    TypeSet<EM::PosID> tmp;
    TypeSet<EM::PosID>& tmpres( res_ ? *res_ : tmp );
    SurfaceGeometry::findPos( sectionid, x, y, z, &tmpres );
    for ( int idx=0; idx<tmpres.size(); idx++ )
    {
	if ( isHidden( tmpres[idx] ) )
	    tmpres.remove(idx--);
    }

    return tmpres.size();
}


}; //namespace

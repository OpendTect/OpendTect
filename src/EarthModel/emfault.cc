/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.33 2005-10-12 20:35:33 cvskris Exp $
________________________________________________________________________

-*/

#include "emfault.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "errh.h"
#include "cubicbeziersurface.h"
#include "survinfo.h"

namespace EM {


const char* Fault::typeStr() { return EMFaultTranslatorGroup::keyword; }


void Fault::initClass(EMManager& emm)
{
    emm.addFactory( new ObjectFactory( create,
				       EMFaultTranslatorGroup::ioContext(),
				       typeStr()) );
}


EMObject* Fault::create( EMManager& emm )
{ return new Fault( emm ); }


Fault::Fault( EMManager& em )
    : Surface(em,*new FaultGeometry(*this))
{
    geometry.addSection( "", false );
}


const IOObjContext& Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


FaultGeometry::FaultGeometry( Fault& flt )
    : SurfaceGeometry(flt)
{
    loadedstep_ = BinID(1,1);
    step_ = BinID(1,1);
}


FaultGeometry::~FaultGeometry()
{}


bool FaultGeometry::isHidden( const PosID& pid ) const
{
    return surface.isPosAttrib( pid, hiddenAttrib() );
}


bool FaultGeometry::isHidden( SectionID sectionid,
			      const RowCol& rc ) const
{
    return isHidden( PosID( surface.id(), sectionid, rc.getSerialized() ) );
}


void FaultGeometry::setHidden( const PosID& pid, bool yn, bool addtohistory)
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


void FaultGeometry::setHidden( SectionID sectionid,
			       const RowCol& rc, bool yn,
			       bool addtohistory )
{
    setHidden( PosID( surface.id(), sectionid, rc.getSerialized()),
	       yn, addtohistory );
}



bool FaultGeometry::insertHiddenColumn( SectionID sectionid, int col)
{
    if ( sectionids.size()>1 )
	pErrMsg( "TODO: Implement support for multiple sections." );

    StepInterval<int> rowrg = rowRange(sectionid);
    if ( rowrg.width(false)<0 )
	return false;

    StepInterval<int> colrg = colRange(sectionid);

    if ( col<colrg.start-1 || col>colrg.stop+1 )
	return false;

    for ( int colidx=colrg.stop; colidx>=col; colidx-=colrg.step )
    {
	for ( int rowidx=rowrg.start; rowidx<=rowrg.stop; rowidx+=rowrg.step )
	{
	    surface.changePosID(
		 PosID( surface.id(),sectionid,
		     RowCol(rowidx,colidx).getSerialized() ),
		 PosID( surface.id(),sectionid,
			RowCol(rowidx,colidx+colrg.step).getSerialized() ),
		 	true );
	}
    }

    for ( int rowidx=rowrg.start; rowidx<=rowrg.stop; rowidx+=rowrg.step )
    {
	const RowCol rc(  RowCol( rowidx, col ) );
	setHidden( sectionid, rc, true, true );
    }

    return true;
}


Coord3 FaultGeometry::getPos( SectionID sectionid, const RowCol& rc ) const
{
    if ( !isHidden(sectionid,rc) )
	return SurfaceGeometry::getPos(sectionid,rc);

    const StepInterval<int> rowrg = rowRange(sectionid);
    if ( rowrg.width(false)<0 ) return Coord3::udf();

    const StepInterval<int> colrg = colRange(sectionid);

    int colstep = step().col;

    int stepout = colstep;
    while ( true )
    {
	const RowCol nextrc(rc.row,rc.col+stepout);
	const RowCol prevrc(rc.row,rc.col-stepout);
	if ( !colrg.includes(nextrc.col) && !colrg.includes(prevrc.col) )
	    break;

	if ( colrg.includes(nextrc.col) && !isHidden(sectionid,nextrc)
		&& isDefined(sectionid,nextrc) )
	    return SurfaceGeometry::getPos(sectionid,nextrc);

	if ( colrg.includes(prevrc.col) && !isHidden(sectionid,prevrc)
		&& isDefined(sectionid,prevrc) )
	    return SurfaceGeometry::getPos(sectionid,prevrc);

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


Geometry::ParametricSurface* FaultGeometry::createSectionSurface() const
{ return new Geometry::CubicBezierSurface( loadedstep_ ); }


PosID FaultGeometry::getNeighbor( const PosID& posid,
				  const RowCol& dir ) const
{
    if ( sectionids.size()>1 )
	pErrMsg( "TODO: Implement support for multiple sections." );

    if ( !dir.col )
	return SurfaceGeometry::getNeighbor( posid, dir );

    const SectionID sectionid = posid.sectionID();

    const StepInterval<int> rowrg = rowRange();
    const StepInterval<int> colrg = colRange();

    RowCol currc( posid.subID() );
    while ( true )
    {
	currc += step_*dir;
	if ( !rowrg.includes(currc.row) || !colrg.includes(currc.col) )
	    break;

	if ( !isHidden( sectionid, currc ) )
	    break;
    }

    return PosID( surface.id(), sectionid, currc.getSerialized() );
}


int FaultGeometry::findPos( SectionID sectionid,
			    const Interval<float>& x,
			    const Interval<float>& y,
			    const Interval<float>& z,
			    TypeSet<PosID>* res_) const
{
    TypeSet<PosID> tmp;
    TypeSet<PosID>& tmpres( res_ ? *res_ : tmp );
    SurfaceGeometry::findPos( sectionid, x, y, z, &tmpres );
    for ( int idx=0; idx<tmpres.size(); idx++ )
    {
	if ( isHidden( tmpres[idx] ) )
	    tmpres.remove(idx--);
    }

    return tmpres.size();
}


}; //namespace

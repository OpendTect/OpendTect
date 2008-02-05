/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.37 2008-02-05 21:46:15 cvskris Exp $
________________________________________________________________________

-*/

#include "emfault.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "errh.h"
#include "cubicbeziersurface.h"
#include "survinfo.h"

namespace EM {

mImplementEMObjFuncs( Fault, EMFaultTranslatorGroup::keyword ) 

Fault::Fault( EMManager& em )
    : Surface(em)
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


Fault::~Fault()
{}


FaultGeometry& Fault::geometry()
{ return geometry_; }


const IOObjContext& Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


FaultGeometry::FaultGeometry( Fault& flt )
    : SurfaceGeometry(flt)
{ }


FaultGeometry::~FaultGeometry()
{}


Geometry::FaultStickSurface*
FaultGeometry::sectionGeometry( const EM::SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::FaultStickSurface*) res;
}


Geometry::FaultStickSurface* FaultGeometry::createSectionGeometry() const
{ return new Geometry::FaultStickSurface; }



EMObjectIterator* FaultGeometry::createIterator( const SectionID& sid,
						 const CubeSampling* cs) const
{ return new RowColIterator( surface_, sid, cs ); }


}; //namespace

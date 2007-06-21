/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.36 2007-06-21 19:35:21 cvskris Exp $
________________________________________________________________________

-*/

#include "emfault.h"
#include "emsurfacetr.h"
#include "emmanager.h"
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
{
}


FaultGeometry::~FaultGeometry()
{}


Geometry::CubicBezierSurface*
FaultGeometry::sectionGeometry( const EM::SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return reinterpret_cast<Geometry::CubicBezierSurface*>( res );
}


Geometry::CubicBezierSurface* FaultGeometry::createSectionGeometry() const
{ return new Geometry::CubicBezierSurface( RowCol(1,1) ); }




}; //namespace

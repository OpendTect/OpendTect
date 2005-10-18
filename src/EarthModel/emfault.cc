/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Fredman
 Date:          Sep 2002
 RCS:           $Id: emfault.cc,v 1.34 2005-10-18 18:34:29 cvskris Exp $
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


Geometry::ParametricSurface* FaultGeometry::createSectionSurface() const
{ return new Geometry::CubicBezierSurface( loadedstep_ ); }




}; //namespace

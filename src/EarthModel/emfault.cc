/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.7 2003-05-05 11:59:55 kristofer Exp $";

#include "emfault.h"

#include "emfaulttransl.h"
#include "emhistoryimpl.h"
#include "geomgridsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

EarthModel::Fault::Fault(EarthModel::EMManager & emm_, const MultiID& mid_)
    : Surface( emm_, mid_ )
{ }


EarthModel::Fault::~Fault()
{ }


Executor* EarthModel::Fault::loader()
{
    if ( isLoaded() ) cleanUp();

    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
Executor* EarthModel::Fault::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


Geometry::GridSurface* EarthModel::Fault::createPatchSurface() const
{
    return new Geometry::GridSurfaceImpl;
}

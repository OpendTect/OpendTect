/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.9 2003-07-30 13:47:27 nanne Exp $";

#include "emfault.h"

#include "emfaulttransl.h"
#include "emhistoryimpl.h"
#include "geomgridsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

EM::Fault::Fault(EM::EMManager & emm_, const MultiID& mid_)
    : Surface( emm_, mid_ )
{ }


EM::Fault::~Fault()
{ }


Executor* EM::Fault::loader()
{
    if ( isLoaded() ) cleanUp();

    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EMFaultTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
Executor* EM::Fault::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EMFaultTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


Geometry::GridSurface* EM::Fault::createPatchSurface( const PatchID& id ) const
{
    return new Geometry::GridSurfaceImpl;
}

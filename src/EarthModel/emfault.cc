/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.14 2003-11-07 12:21:57 bert Exp $";

#include "emfault.h"

#include "emfaulttransl.h"
#include "emhistoryimpl.h"
#include "geommeshsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

EM::Fault::Fault(EM::EMManager & emm_, const MultiID& mid_)
    : Surface( emm_, mid_ )
{ }


EM::Fault::~Fault()
{ }


Executor* EM::Fault::loader( const EM::SurfaceIODataSelection* newsel,
			     int attridx )
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

    
Executor* EM::Fault::saver( const EM::SurfaceIODataSelection* newsel,
			    bool auxdata, const MultiID* key )
{
    const MultiID& mid = key ? *key : id();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    Executor* exec = EMFaultTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}


Geometry::MeshSurface* EM::Fault::createPatchSurface( const PatchID& id ) const
{
    return new Geometry::MeshSurfaceImpl;
}

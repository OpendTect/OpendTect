#ifndef raytracerrunner_h
#define raytracerrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: raytracerrunner.h,v 1.5 2011-10-12 11:32:33 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "executor.h"
#include "ranges.h"
#include "raytrace1d.h"

mClass RayTracerRunner : public Executor
{
public:
    				RayTracerRunner(const TypeSet<ElasticModel>&,
						const IOPar& raypar);
    				~RayTracerRunner();

    int                         nextStep();
    od_int64                    totalNr() const { return aimodels_.size(); }
    od_int64                    nrDone() const  { return nrdone_; }
    const char*                 message() const { return "Running Ray tracers";}
    const char*			errMsg() const 	{ return errmsg_.buf(); }

    //available after excution
    ObjectSet<RayTracer1D>& 	rayTracers() 	{ return raytracers_; }

protected:

    int 			nrdone_;
    const IOPar&		raypar_;

    BufferString		errmsg_;

    const TypeSet<ElasticModel>& aimodels_;
    ObjectSet<RayTracer1D> 	raytracers_;
};

#endif

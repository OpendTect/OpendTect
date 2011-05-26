#ifndef raytracerrunner_h
#define raytracerrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: raytracerrunner.h,v 1.3 2011-05-26 15:42:47 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "executor.h"
#include "ranges.h"
#include "raytrace1d.h"

mClass RayTracerRunner : public Executor
{
public:
    				RayTracerRunner(const TypeSet<AIModel>&,
						const TypeSet<float>& offs,
						const RayTracer1D::Setup&);
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
    RayTracer1D::Setup          raysetup_;
    TypeSet<float>              offsets_;

    BufferString		errmsg_;

    const TypeSet<AIModel>&	aimodels_;
    ObjectSet<RayTracer1D> 	raytracers_;
};

#endif


#ifndef raytracerrunner_h
#define raytracerrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id: raytracerrunner.h,v 1.2 2011-05-25 15:49:02 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "executor.h"
#include "ranges.h"
#include "raytrace1d.h"

mClass RayTracerRunner : public Executor
{
public:
    				RayTracerRunner(const ObjectSet<AIModel>&,
						const TypeSet<float>& offs,
						const RayTracer1D::Setup&);

    int                         nextStep();
    od_int64                    totalNr() const { return aimodels_.size(); }
    od_int64                    nrDone() const  { return nrdone_; }
    const char*                 message() const { return "Running Ray tracers";}

    //available after excution
    const RayTracer1D*		rayTracer(int idx) const; 
    const char*			errMsg() const 	{ return errmsg_.buf(); }

protected:

    int 			nrdone_;
    RayTracer1D::Setup          raysetup_;
    TypeSet<float>              offsets_;

    BufferString		errmsg_;

    const ObjectSet<AIModel>&	aimodels_;
    ObjectSet<RayTracer1D> 	raytracers_;
};

#endif


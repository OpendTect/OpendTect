#ifndef raytracerrunner_h
#define raytracerrunner_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "ailayer.h"
#include "iopar.h"
#include "task.h"
#include "ranges.h"
#include "raytrace1d.h"

mExpClass(General) RayTracerRunner : public ParallelTask
{
public:
    				RayTracerRunner(const TypeSet<ElasticModel>&,
						const IOPar& raypar);
    				RayTracerRunner(const IOPar& raypar);
    				~RayTracerRunner();

    const char*			errMsg() const 	{ return errmsg_.buf(); }

    //before exectution only
    void			setOffsets(TypeSet<float> offsets);
    void			addModel(const ElasticModel&,bool dosingle); 
    bool                        prepareRayTracers();

    //available after excution
    ObjectSet<RayTracer1D>& 	rayTracers() 	{ return raytracers_; }
    od_int64			nrDone() const;

protected:

    IOPar			raypar_;

    bool                	doWork(od_int64,od_int64,int);
    od_int64                    nrIterations() const;
    int 			curModelIdx(od_int64) const;

    BufferString		errmsg_;

    TypeSet<ElasticModel> aimodels_;
    ObjectSet<RayTracer1D> 	raytracers_;
    od_int64			totalnr_;
};

#endif


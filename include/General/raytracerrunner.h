#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "generalmod.h"
#include "ailayer.h"
#include "iopar.h"
#include "task.h"
#include "ranges.h"
#include "raytrace1d.h"

mExpClass(General) RayTracerRunner : public ParallelTask
{ mODTextTranslationClass(RayTracerRunner);
public:
				RayTracerRunner(const TypeSet<ElasticModel>&,
						const IOPar& raypar);
				RayTracerRunner(const IOPar& raypar);
				~RayTracerRunner();

    uiString			errMsg() const	{ return errmsg_; }

    //before exectution only
    void			setOffsets(TypeSet<float> offsets);
    void			addModel(const ElasticModel&,bool dosingle);

    //available after excution
    ObjectSet<RayTracer1D>&	rayTracers()	{ return raytracers_; }
    od_int64			nrDone() const;
    bool			executeParallel(bool);

protected:

    IOPar			raypar_;

    bool	doWork(od_int64,od_int64,int);
    od_int64                    nrIterations() const;
    int			modelIdx(od_int64,bool&) const;
    bool                        prepareRayTracers();

    uiString			errmsg_;

    TypeSet<ElasticModel>	aimodels_;
    ObjectSet<RayTracer1D>	raytracers_;
    od_int64			totalnr_;
};


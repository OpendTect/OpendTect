#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "generalmod.h"

#include "iopar.h"
#include "manobjectset.h"
#include "task.h"

class ElasticModelSet;
class RayTracer1D;
class RayTracerData;



mExpClass(General) RayTracerRunner : public TaskGroup
{ mODTextTranslationClass(RayTracerRunner)
public:

    typedef RefObjectSet<RayTracerData>		RayTracerDataSet;

				RayTracerRunner(const ElasticModelSet&,
						const IOPar& raypar);
				RayTracerRunner(const IOPar& raypar);
    virtual			~RayTracerRunner();

    void			setOffsets(const TypeSet<float>&);
    void			setModel(const ElasticModelSet&);
    void			setParallel(bool yn)	{ parallel_ = yn; }

    virtual bool		execute() final;

    uiString			errMsg() const		{ return errmsg_; }

    const RayTracerDataSet&	results() const		{ return results_; }

private:

    bool			doPrepare();
    bool			doFinish(bool);

    IOPar			raypar_;
    bool			parallel_ = true;
    uiString			errmsg_;

    const ElasticModelSet*	elasticmodels_;
    ObjectSet<RayTracer1D>	raytracers_;
    RayTracerDataSet		results_;

};

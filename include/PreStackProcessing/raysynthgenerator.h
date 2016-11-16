#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "syntheticdata.h"
#include "synthseis.h"

class SeisTrc;
class SeisTrcBuf;
class RayTracerRunner;
class TimeDepthModel;
class DataPack;


/*!\brief base class for synthetic trace generators. */

mExpClass(PreStackProcessing) RaySynthGenerator : public ParallelTask
						, public Seis::SynthGenBase
{ mODTextTranslationClass(RaySynthGenerator);
public:

			RaySynthGenerator(const TypeSet<ElasticModel>*,
					  const SynthGenParams&);
			RaySynthGenerator(SyntheticData*, bool overwrite);
			~RaySynthGenerator();

    void		reset();

    //input
    void		fillPar(IOPar& raypars) const;
    bool		usePar(const IOPar& raypars);
    void		forceReflTimes(const StepInterval<float>&);

    //available after initialization
    void		getAllRefls(RefMan<ReflectivityModelSet>&);

    uiString		message() const
			{ return errmsg_.isEmpty() ? message_ : errmsg_; }


    //available after execution
    SyntheticData::RayModel&		result(int id)
			{ return *(*synthdata_->raymodels_)[id]; }
    const SyntheticData::RayModel&	result(int id) const
			{ return *(*synthdata_->raymodels_)[id]; }

    const ObjectSet<RayTracer1D>& rayTracers() const;
    const TypeSet<ElasticModel>& elasticModels() const	{ return *aimodels_; }
    void		getTraces(ObjectSet<SeisTrcBuf>&);
    void		getStackedTraces(SeisTrcBuf&);
    SyntheticData*	getSyntheticData() const { return synthdata_; }
    bool		updateDataPack();

protected:
    RayTracerRunner*		rtr_;
    od_int64			nrIterations() const;
    od_int64			nrDone() const;
    uiString			nrDoneText() const;
    od_int64			totalNr() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    bool			ownraymodels_;
    uiString			message_;
    const TypeSet<ElasticModel>* aimodels_;
    TypeSet<float>		offsets_;
    IOPar			raysetup_;

    StepInterval<float>		forcedrefltimes_;
    bool			forcerefltimes_;
    bool			raytracingdone_;
    bool			overwrite_;
    SyntheticData*		synthdata_;
};

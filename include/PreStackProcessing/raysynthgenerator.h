#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "synthseis.h"

class SeisTrc;
class SeisTrcBuf;
class RayTracerRunner;
class TimeDepthModel;


/*!\brief base class for synthetic trace generators. */

mExpClass(PreStackProcessing) RaySynthGenerator : public ParallelTask
						, public Seis::SynthGenBase
{ mODTextTranslationClass(RaySynthGenerator);
public:

    mStruct(PreStackProcessing) RayModel
    {
			RayModel(const RayTracer1D& rt1d,int nroffsets);
			~RayModel();

	void		getTraces(ObjectSet<SeisTrc>&,bool steal);
	void		getD2T(ObjectSet<TimeDepthModel>&,bool steal);
	void		getZeroOffsetD2T(TimeDepthModel&);
	RefMan<ReflectivityModelSet>&	getRefs(bool sampled=false);
	void		forceReflTimes(const StepInterval<float>&);

	const SeisTrc*	stackedTrc() const;

    protected:
	ObjectSet<SeisTrc>			outtrcs_; //this is a gather
	ObjectSet<TimeDepthModel>		t2dmodels_;
	TimeDepthModel*				zerooffset2dmodel_;

	RefMan<ReflectivityModelSet>		refmodels_;
	RefMan<ReflectivityModelSet>		sampledrefmodels_;

	friend class				RaySynthGenerator;

    };

			RaySynthGenerator(const TypeSet<ElasticModel>*,
					  bool ownrms=true);
			RaySynthGenerator(ObjectSet<RayModel>*);
			~RaySynthGenerator();

    void		reset();

    //input
    void		fillPar(IOPar& raypars) const;
    bool		usePar(const IOPar& raypars);
    void		forceReflTimes(const StepInterval<float>&);

    //available after initialization
    void		getAllRefls(RefMan<ReflectivityModelSet>&,
				    bool sampled=false);

    uiString		uiMessage() const
			{ return errmsg_.isEmpty() ? message_ : errmsg_; }


    //available after execution
    RayModel&		result(int id)		{ return *(*raymodels_)[id]; }
    const RayModel&	result(int id) const	{ return *(*raymodels_)[id]; }
    ObjectSet<RayModel>* rayModels()		{ return raymodels_; }

    const ObjectSet<RayTracer1D>& rayTracers() const;
    const TypeSet<ElasticModel>& elasticModels() const	{ return *aimodels_; }
    void		getTraces(ObjectSet<SeisTrcBuf>&);
    void		getStackedTraces(SeisTrcBuf&);

protected:
    RayTracerRunner*		rtr_;
    od_int64			nrIterations() const;
    od_int64			nrDone() const;
    uiString			uiNrDoneText() const;
    od_int64			totalNr() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);

    bool			ownraymodels_;
    uiString			message_;
    const TypeSet<ElasticModel>* aimodels_;
    TypeSet<float>		offsets_;
    IOPar			raysetup_;
    ObjectSet<RayModel>*	raymodels_;

    StepInterval<float>		forcedrefltimes_;
    bool			forcerefltimes_;
    bool			raytracingdone_;
};

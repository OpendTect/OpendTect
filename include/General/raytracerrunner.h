#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "raytrace1d.h"

class ElasticModelSet;
class IOPar;

/*!
  A task to compute reflectivities from a set of ElasticModel objects,
  using the RayTracer1D factory.
  The results are stored as a set of ReflectivityModel objects,
  which combine time-depth models and associated reflectivities.
  Like for the TimeDepthModel class, the output depths units correspond to
  SI().depthsInFeet(), and are TVDSS depths (0 at sea-level, not at SRD,
  positive below sea-level and increasing downwards.
  The first layer of each ElasticModel must be a SRD.
 */


mExpClass(General) RayTracerRunner : public ParallelTask
{ mODTextTranslationClass(RayTracerRunner);
public:
				RayTracerRunner(const char* rt1dfactkeywd=
					VrmsRayTracer1D::sFactoryKeyword());
				RayTracerRunner(const IOPar& raypar);
				mDeprecated("Provide setup")
				RayTracerRunner(const ElasticModelSet&,
						const IOPar& raypar);
				RayTracerRunner(const ElasticModelSet&,
						const IOPar& raypar,
						const RayTracer1D::Setup*);
				~RayTracerRunner();

    //before execution only
				mDeprecated("Provide setup")
    bool			setModel(const ElasticModelSet&);
    bool			setModel(const ElasticModelSet&,
					 const RayTracer1D::Setup*);
				mDeprecated("Use Seis::OffsetType")
    void			setOffsets(const TypeSet<float>&);
    void			setOffsets(const TypeSet<float>&,
					   Seis::OffsetType);

    uiString			uiMessage() const override { return msg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override;

    static const char*		sKeyParallel();

    //available after execution
    ConstRefMan<ReflectivityModelSet> getRefModels() const;

				mDeprecated("Provide setup")
    static ConstRefMan<ReflectivityModelSet> getRefModels(
				    const ElasticModelSet&,
				    const IOPar& raypar,uiString& msg,
				    TaskRunner* =nullptr,
			    const ObjectSet<const TimeDepthModel>* =nullptr);
    static ConstRefMan<ReflectivityModelSet> getRefModels(
				    const ElasticModelSet&,
				    const IOPar& raypar,uiString& msg,
				    const RayTracer1D::Setup*,
				    TaskRunner* =nullptr,
			    const ObjectSet<const TimeDepthModel>* =nullptr);
private:

    IOPar&			raypar_;

    od_int64                    nrIterations() const override;

    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    int				modelIdx(od_int64,bool& startlayer) const;
    void			computeTotalNr();

    bool			getResults(ReflectivityModelSet&) const;

    uiString			msg_;

    ObjectSet<RayTracer1D>	raytracers_;
    od_int64			totalnr_;

};

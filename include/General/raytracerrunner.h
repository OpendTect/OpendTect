#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "generalmod.h"

#include "raytrace1d.h"

class ElasticModel;
class IOPar;

mExpClass(General) RayTracerRunner : public ParallelTask
{ mODTextTranslationClass(RayTracerRunner);
public:
				RayTracerRunner(const char* rt1dfactkeywd=
					VrmsRayTracer1D::sFactoryKeyword());
				RayTracerRunner(const IOPar& raypar);
				RayTracerRunner(const TypeSet<ElasticModel>&,
						const IOPar& raypar);
				~RayTracerRunner();

    //before execution only
    bool			setModel(const TypeSet<ElasticModel>&);
				//<! No copy: Must stay valid during execution
    void			setOffsets(const TypeSet<float>&);


    uiString			uiMessage() const override { return msg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override;

    static const char*		sKeyParallel();

    //available after execution
    ConstRefMan<ReflectivityModelSet> getRefModels() const;

    static ConstRefMan<ReflectivityModelSet> getRefModels(
				    const TypeSet<ElasticModel>&,
				    const IOPar& raypar,uiString& msg,
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


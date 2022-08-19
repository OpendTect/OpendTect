#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "reflcalc1d.h"

class ElasticModel;
class IOPar;

mExpClass(General) ReflCalcRunner : public ParallelTask
{ mODTextTranslationClass(ReflCalcRunner);
public:
				ReflCalcRunner(const char* refl1dfactkeywd=
					       AICalc1D::sFactoryKeyword());
				ReflCalcRunner(const IOPar& reflpar);
				ReflCalcRunner(const TypeSet<ElasticModel>&,
					       const IOPar& reflpar);
				~ReflCalcRunner();

    //before execution only
    bool			setModel(const TypeSet<ElasticModel>&);
				//<! No copy: Must stay valid during execution
    void			setAngle(float thetaang,bool angleisindegrees);
    void			setAngles(const TypeSet<float>&,
					  bool angleisindegrees);

    uiString			uiMessage() const override { return msg_; }
    uiString			uiNrDoneText() const override;
    od_int64			nrDone() const override;

    static const char*		sKeyParallel();

    //available after execution
    ConstRefMan<ReflectivityModelSet> getRefModels() const;

    static ConstRefMan<ReflectivityModelSet> getRefModels(
				    const TypeSet<ElasticModel>&,
				    const IOPar& reflpar,uiString& msg,
				    TaskRunner* =nullptr,
			    const ObjectSet<const TimeDepthModel>* =nullptr);
private:

    IOPar&			reflpar_;

    od_int64			nrIterations() const override;

    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    int				modelIdx(od_int64,bool& startlayer) const;
    void			computeTotalNr();

    bool			getResults(ReflectivityModelSet&) const;

    uiString			msg_;

    ObjectSet<ReflCalc1D>	reflcalcs_;
    od_int64			totalnr_;

};

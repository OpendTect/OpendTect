#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "reflcalc1d.h"

class ElasticModelSet;
class IOPar;

/*!
  A task to compute reflectivities from a set of ElasticModel objects,
  using the ReflCalc1D factory.
  The results are stored as a set of ReflectivityModel objects,
  which combine time-depth models and associated reflectivities.
  Like for the TimeDepthModel class, the output depths units correspond to
  SI().depthsInFeet(), and are TVDSS depths (0 at sea-level, not at SRD,
  positive below sea-level and increasing downwards.
  The first layer of each ElasticModel must be a SRD.
 */

mExpClass(General) ReflCalcRunner : public ParallelTask
{ mODTextTranslationClass(ReflCalcRunner);
public:
				ReflCalcRunner(const char* refl1dfactkeywd=
					       AICalc1D::sFactoryKeyword());
				ReflCalcRunner(const IOPar& reflpar);
				ReflCalcRunner(const ElasticModelSet&,
					       const IOPar& reflpar,
					       const ReflCalc1D::Setup*);
				~ReflCalcRunner();

    //before execution only
    bool			setModel(const ElasticModelSet&,
					 const ReflCalc1D::Setup*);
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
				    const ElasticModelSet&,
				    const IOPar& reflpar,uiString& msg,
				    const ReflCalc1D::Setup* =nullptr,
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

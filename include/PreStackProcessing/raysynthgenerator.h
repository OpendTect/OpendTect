#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "synthseisdataset.h"
#include "synthseisgenerator.h"

class DataPack;
class ElasticModelSet;
class RayTracerRunner;
class SeisTrc;
class SeisTrcBuf;
class TimeDepthModel;


/*!\brief base class for synthetic trace generators. */

mExpClass(PreStackProcessing) RaySynthGenerator : public ParallelTask
						, public SynthSeis::GenBase
{ mODTextTranslationClass(RaySynthGenerator);
public:

    typedef SynthSeis::DataSet		DataSet;
    typedef SynthSeis::RayModel		RayModel;
    typedef SynthSeis::GenParams	GenParams;
    typedef ObjectSet<RayModel>		RayModelSet;

			RaySynthGenerator(const GenParams&,
					  const ElasticModelSet&);
			RaySynthGenerator(const GenParams&,
					  const RayModelSet&);
			~RaySynthGenerator();

    void		reset();
    void		setNrStep( int stp )	{ nrstep_ = stp; }

    //input
    void		fillPar(IOPar& raypars) const;
    bool		usePar(const IOPar& raypars);
    void		forceReflTimes(const StepInterval<float>&);

    uiString		message() const
			{ return errmsg_.isEmpty() ? message_ : errmsg_; }

    //available after execution
    RayModel&		result( int idx )
			{ return *dataset_->rayMdls()[idx]; }
    const RayModel&	result( int idx ) const
			{ return *dataset_->rayModels()[idx]; }

    const ObjectSet<RayTracer1D>& rayTracers() const;
    const ElasticModelSet& elasticModels() const;
    void		getTraces(ObjectSet<SeisTrcBuf>&);
    void		getStackedTraces(SeisTrcBuf&);
    DataSet*		dataSet()		{ return dataset_; }
    const DataSet*	dataSet() const		{ return dataset_; }
    bool		updateDataPack();

protected:

    od_int64			nrIterations() const;
    od_int64			nrDone() const;
    uiString			nrDoneText() const;
    od_int64			totalNr() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    RayTracerRunner*		rtr_;
    bool			ownraymodels_;
    uiString			message_;
    const ElasticModelSet*	elasticmodels_	    = 0;
    TypeSet<float>		offsets_;
    IOPar			raysetup_;
    int				nrstep_		    = 1;

    StepInterval<float>		forcedrefltimes_;
    bool			forcerefltimes_;
    bool			raytracingdone_;
    DataSet*			dataset_;

private:

    void			createDataSet(const GenParams&);

};

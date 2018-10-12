#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "seisbuf.h"
#include "synthseisdataset.h"
#include "synthseisgenerator.h"

class DataPack;
class ElasticModelSet;
class SeisTrc;
class SeisTrcBuf;
class TimeDepthModel;
namespace SynthSeis { class RayModelSet; }


/*!\brief base class for synthetic trace generators. */

mExpClass(PreStackProcessing) RaySynthGenerator : public ParallelTask
						, public SynthSeis::GenBase
{ mODTextTranslationClass(RaySynthGenerator);
public:

    typedef SynthSeis::DataSet		DataSet;
    typedef SynthSeis::RayModel		RayModel;
    typedef SynthSeis::RayModelSet	RayModelSet;
    typedef SynthSeis::GenParams	GenParams;

			RaySynthGenerator(const GenParams&,RayModelSet&);
			RaySynthGenerator(DataSet&);
    virtual		~RaySynthGenerator();

    virtual uiString	message() const final	{ return GenBase::message(); }
    virtual uiString	nrDoneText() const final;

    void		setNrStep( int stp )		{ nrstep_ = stp; }

    bool		isResultOK() const;

    const DataSet&	dataSet() const		{ return *dataset_; }
    DataSet&		dataSet()		{ return *dataset_; }

protected:

    virtual od_int64	nrIterations() const final;

    int			nrstep_		    = 1;
    RefMan<DataSet>	dataset_;

private:

    virtual bool	doPrepare(int) final;
    virtual bool	doWork(od_int64,od_int64,int) final;
    virtual bool	doFinish(bool) final;

    void		createDataSet(const GenParams&,RayModelSet&);

    bool		updateDataPack();

    const RayModelSet&	rayModels() const;
    RayModelSet&	rayMdls()	{ return dataset_->rayMdls();}

    ManagedObjectSet<SeisTrcBuf> trcsset_;
    ObjectSet<SynthSeis::MultiTraceGenerator> generators_;

};

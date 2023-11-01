#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "paralleltask.h"
#include "seistype.h"
#include "thread.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

class IOObj;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class SeisSequentialWriter;
class VelocityDesc;
class ZAxisTransformSampler;
namespace Vel { class Worker; }
template <class yT,class xT> class MathFunction;


/*!
  brief Stretches the zaxis from the input cube with a ZAxisTransform and
  writes it out into another volume.
 */

mExpClass(Seis) SeisZAxisStretcherNew : public ParallelTask
{ mODTextTranslationClass(SeisZAxisStretcherNew);
public:
			SeisZAxisStretcherNew(const IOObj& in,const IOObj& out,
					      const TrcKeyZSampling& outcs,
					      ZAxisTransform&,bool forward,
					      const VelocityDesc* =nullptr);
			~SeisZAxisStretcherNew();

    bool		isOK() const;

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    void		setUdfVal(float val);

private:

    void		setRanges();
    bool		init();
    void		setWorkers(const VelocityDesc&);

    od_int64		nrIterations() const override	{ return totalnr_; }
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    bool		getInputTrace(SeisTrc&);
    bool		loadTransformChunk(int firstinl);
    void		stretch(const SeisTrc&,int icomp,
				const MathFunction<float,float>& intrcfunc,
				ZAxisTransformSampler&,float* tmpptr,
				SeisTrc&) const;
    bool		selfStretchVelocity(const SeisTrc&,int icomp,
					    SeisTrc&) const;
    bool		stretchVelocity(const SeisTrc&,int icomp,
					SeisTrc&) const;

    const IOObj&			inpobj_;
    const IOObj&			outobj_;

    SeisTrcReader*			seisreader_ = nullptr;
    Threads::ConditionVar		readerlock_;
    Threads::ConditionVar		readerlockmodel_;

    SeisTrcWriter*			seiswriter_ = nullptr;
    SeisSequentialWriter*		sequentialwriter_ = nullptr;
    bool				waitforall_ = false;
    int					nrwaiting_ = 0;
    int					nrthreads_ = 0;

    TrcKeyZSampling			outcs_;
    TrcKeySampling			curhrg_;
    RefMan<ZAxisTransform>		ztransform_;
    uiString				msg_;
    int					totalnr_ = -1;
    int					voiid_ = -1;
    const bool				ist2d_;
    Seis::GeomType			geomtype_;
    float				udfval_		= mUdf(float);

    Vel::Worker*			worker_		= nullptr;
    Vel::Worker*			vintworker_	= nullptr;
    bool				autotransform_	= false;
    const ZDomain::Info*		zdomaininfo_ = nullptr;
    double				srd_;
    const UnitOfMeasure*		srduom_;

};


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. If stretchinverse is true, the stretching will
   be done on the inveres of the values.
 */

mExpClass(Seis) SeisZAxisStretcher : public ParallelTask
{ mODTextTranslationClass(SeisZAxisStretcher);
public:
			mDeprecated("Use SeisZAxisStretcherNew")
			SeisZAxisStretcher( const IOObj& in,
					     const IOObj& out,
					     const TrcKeyZSampling& outcs,
					     ZAxisTransform&,
					     bool forward,
					     bool stretchz);
			~SeisZAxisStretcher();

    bool		isOK() const;

    void		setGeomID(Pos::GeomID);
    uiString		uiMessage() const override
			{ return tr("Stretching data"); }
    uiString		uiNrDoneText() const override
			{ return tr("Traces done"); }

    void		setVelTypeIsVint( bool yn )	{ isvint_ = yn; }
    void		setVelTypeIsVrms( bool yn )	{ isvrms_ = yn; }
    void		setUdfVal(float val);

protected:

    void		init(const IOObj& in,const IOObj& out);
    bool		doPrepare(int) override;
    bool		doFinish(bool) override;
    bool		doWork(od_int64,od_int64,int) override;
    od_int64		nrIterations() const override	{ return totalnr_; }

    bool		getInputTrace(SeisTrc&,TrcKey&);
    bool		getModelTrace(SeisTrc&,TrcKey&);
    bool		loadTransformChunk(int firstinl);
    bool		doZStretch(SeisTrc& intrc,SeisTrc& outtrc,
				   const TrcKey&,const SamplingData<float>&);


    SeisTrcReader*			seisreader_;
    Threads::ConditionVar		readerlock_;
    Threads::ConditionVar		readerlockmodel_;

    SeisTrcReader*			seisreadertdmodel_;

    SeisTrcWriter*			seiswriter_;
    SeisSequentialWriter*		sequentialwriter_;
    bool				waitforall_;
    int					nrwaiting_;
    int					nrthreads_;

    TrcKeyZSampling			outcs_;
    TrcKeySampling			curhrg_;
    ZAxisTransform*			ztransform_;
    int					voiid_;
    bool				ist2d_;
    bool				is2d_;
    bool				stretchz_;

    int					totalnr_;
    bool				isvint_;
    bool				isvrms_;
    float				udfval_		= mUdf(float);

};

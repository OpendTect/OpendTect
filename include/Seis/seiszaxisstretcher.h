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

mExpClass(Seis) SeisZAxisStretcher : public ParallelTask
{ mODTextTranslationClass(SeisZAxisStretcher);
public:
			SeisZAxisStretcher( const IOObj& in,const IOObj& out,
					    const TrcKeyZSampling& outcs,
					    ZAxisTransform&,bool forward,
					    const VelocityDesc* =nullptr);

			mDeprecated("Use VelocityDesc")
			SeisZAxisStretcher( const IOObj& in,const IOObj& out,
					    const TrcKeyZSampling& outcs,
					    ZAxisTransform&,bool forward,
					    bool stretchvels);
			~SeisZAxisStretcher();

    bool		isOK() const;

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    void		setUdfVal(float val);

    mDeprecated("GeomID is read from outcs in constructor")
    void		setGeomID(Pos::GeomID);
    mDeprecated("Provide VelocityDesc on construction")
    void		setVelTypeIsVint(bool yn);
    mDeprecated("Provide VelocityDesc on construction")
    void		setVelTypeIsVrms(bool yn);

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

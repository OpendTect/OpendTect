#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "iopar.h"
#include "prestackprocessor.h"
#include "velocityfunctionvolume.h"

class ElasticModel;
class Muter;
class RayTracerRunner;
class ReflectivityModelBase;

namespace PreStack
{

/*!
\brief Base class for AngleMute and AngleMuteComputer.
*/

mExpClass(PreStackProcessing) AngleCompParams
{
public:
				AngleCompParams();
				~AngleCompParams();

    float			mutecutoff_ = 30.f;
    Interval<int>		anglerange_;
    MultiID			velvolmid_;
    IOPar			raypar_;
    IOPar			smoothingpar_;
};


mExpClass(PreStackProcessing) AngleMuteBase
{
public:

    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			AngleMuteBase();
    virtual		~AngleMuteBase();

    bool		setVelocityFunction();
    virtual void	block(ElasticModel&) const		{}
    bool		getLayers(const BinID&,ElasticModel&,
				  SamplingData<float>&,int resamplesz=-1);
    float		getOffsetMuteLayer(const ReflectivityModelBase&,
					   int nrlayer,int ioff,bool tail,
					   int startlayer=0,
					   bool belowcutoff=true) const;

    AngleCompParams*	params_ = nullptr;
    RefMan<Vel::VolumeFunctionSource>	velsource_;
    ObjectSet<RayTracerRunner>	rtrunners_;
};


/*!
\brief Angle mute
*/

mExpClass(PreStackProcessing) AngleMute : public Processor, public AngleMuteBase
{ mODTextTranslationClass(AngleMute);
public:
			mDefaultFactoryInstantiation(Processor,
				AngleMute,"AngleMute", tr("Angle Mute") )

			AngleMute();
			~AngleMute();

    mStruct(PreStackProcessing) AngleMutePars : public AngleCompParams
    {
			AngleMutePars();
			~AngleMutePars();

	bool		tail_ = false;
	float		taperlen_ = 10.f;
    };

    uiString		errMsg() const override		{ return errmsg_; }

    AngleMutePars&	params();
    const AngleMutePars& params() const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:

    bool		doRayTraceParallel() const { return raytraceparallel_; }
    Muter*		getMuter(int);
    uiString		errmsg_;

private:

    od_int64		nrIterations() const override
			{ return outputs_.size(); }

    bool		doPrepare(int nrthreads) override;
    bool		doWork(od_int64,od_int64,int) override;

    bool		raytraceparallel_;
    ObjectSet<Muter>	muters_;
};

} // namespace PreStack

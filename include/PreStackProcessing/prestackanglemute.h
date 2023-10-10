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
#include "raytrace1d.h"
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
    RayTracer1D::Setup		rtsu_;
    IOPar			raypar_;
    IOPar			smoothingpar_;
    float			maxthickness_ = 25.f;
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
    virtual void	block(ElasticModel&) const;
    bool		getLayers(const TrcKey&,ElasticModel&,uiString& errmsg);
    float		getOffsetMuteLayer(const ReflectivityModelBase&,
					   int ioff,bool innermute,
					   bool& nonemuted,bool& allmuted,
					   TypeSet< Interval<float> >&) const;

    float		getfMutePos(const TimeDepthModel&,bool intime,
				    float offsetmutelayer,float offset) const;

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

			AngleMute(const char* nm=sFactoryKeyword());
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

    static void		removeClass();

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

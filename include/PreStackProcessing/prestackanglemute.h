#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"

#include "iopar.h"
#include "prestackprocessor.h"
#include "samplingdata.h"

class ElasticLayer;
class ElasticModel;
class Muter;
class RayTracerData;
class RayTracerRunner;
namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{

/*!
\brief Base class for AngleMute and AngleMuteComputer.
*/

mExpClass(PreStackProcessing) AngleCompParams
{
public:
				AngleCompParams();

    float			mutecutoff_;
    Interval<int>		anglerange_;
    DBKey			velvolmid_;
    IOPar			raypar_;
    IOPar			smoothingpar_;
};


mExpClass(PreStackProcessing) AngleMuteBase
{ mODTextTranslationClass(AngleMuteBase)
public:

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			AngleMuteBase();
			~AngleMuteBase();

    bool		setVelocityFunction();
    bool		getLayers(const BinID&,ElasticModel&,
				SamplingData<float>&,int resamplesz=-1);
    float		getOffsetMuteLayer(const RayTracerData&,int,int,bool,
				int startlayer=0,bool belowcutoff=true) const;

    mutable uiString	msg_;
    AngleCompParams*	params_;
    Vel::VolumeFunctionSource*	velsource_;
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
			AngleMutePars()
			    : tail_(false)
			    , taperlen_(10)
			    {}

	bool		tail_;
	float		taperlen_;
    };

    virtual uiString	message() const final	{ return msg_; }

    AngleMutePars&	 params();
    const AngleMutePars& params() const;

    static const char*  sKeyTaperLength()	{ return "Taper lenght"; }
    static const char*  sKeyIsTail()		{ return "Mute tail"; }

    virtual void	fillPar(IOPar&) const final;
    virtual bool	usePar(const IOPar&) final;

protected:

    virtual od_int64	nrIterations() const final { return outputs_.size(); }

    bool		raytraceparallel_;
    ObjectSet<Muter>	muters_;

private:
    virtual bool	doPrepare(int) final;
    virtual bool	doWork(od_int64,od_int64,int);
    virtual bool	doFinish(bool) final;
};

} // namespace PreStack

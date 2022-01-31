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

    float			mutecutoff_ = 30.f;
    Interval<int>		anglerange_;
    MultiID			velvolmid_;
    IOPar			raypar_;
    IOPar			smoothingpar_;
};


mExpClass(PreStackProcessing) AngleMuteBase
{
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
			AngleMutePars()
			    : tail_(false)
			    , taperlen_(10)
			    {}

	bool		tail_;
	float		taperlen_;
    };

    bool		doPrepare(int nrthreads);

    uiString		errMsg() const		{ return errmsg_; }

    AngleMutePars&	 params();
    const AngleMutePars& params() const;

    static const char*  sKeyTaperLength()	{ return "Taper lenght"; }
    static const char*  sKeyIsTail()		{ return "Mute tail"; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    od_int64		nrIterations() const	{ return outputs_.size(); }
    virtual bool	doWork(od_int64,od_int64,int);

    uiString		errmsg_;
    bool		raytraceparallel_;
    ObjectSet<Muter>	muters_;
};

} // namespace PreStack


#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "iopar.h"
#include "samplingdata.h"

class ElasticLayer;
class ElasticModel;
class MultiID;
class Muter;
class RayTracer1D;
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
					
    float 			mutecutoff_;
    Interval<int>		anglerange_;
    MultiID			velvolmid_;   
    IOPar			raypar_;
    IOPar			smoothingpar_;
};

mExpClass(PreStackProcessing) AngleMuteBase 
{
public:

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }	
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

protected:
    			AngleMuteBase();
    			~AngleMuteBase();

    AngleCompParams*		params_;

    Vel::VolumeFunctionSource*	velsource_;

    bool	setVelocityFunction();
    bool	getLayers(const BinID&,ElasticModel&,
	    			SamplingData<float>&,int resamplesz=-1);
    float	getOffsetMuteLayer(const RayTracer1D&,int,int,bool,
				int startlayer=0,bool belowcutoff=true) const;

    ObjectSet<RayTracerRunner>	rtrunners_;
};


/*!
\brief Angle mute
*/

mExpClass(PreStackProcessing) AngleMute : public Processor, public AngleMuteBase
{
public:
    			mDefaultFactoryInstantiation(Processor,
				AngleMute,"AngleMute", "Angle Mute" );

			AngleMute();
			~AngleMute();

    mStruct(PreStackProcessing) AngleMutePars : public AngleCompParams
    {
			AngleMutePars()
			    : tail_(false)
			    , taperlen_(10) 
			    {}

	bool 		tail_;
	float 		taperlen_;			    
    };
    static const char*  sKeyTaperLength()	{ return "Taper lenght"; }
    static const char*  sKeyIsTail()		{ return "Mute tail"; }

    bool		doPrepare(int nrthreads);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    const char*         errMsg() const 
    			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }

    AngleMutePars&	 params();
    const AngleMutePars& params() const;

protected:

    od_int64 		nrIterations() const	{ return outputs_.size(); }
    virtual bool	doWork(od_int64,od_int64,int);

    BufferString	errmsg_;
    bool		raytraceparallel_;
    ObjectSet<Muter>	muters_;
};


}; //namespace

#endif



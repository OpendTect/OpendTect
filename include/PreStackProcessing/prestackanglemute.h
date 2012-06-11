#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.14 2012-06-11 19:16:27 cvsbruno Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "iopar.h"
#include "samplingdata.h"

class ElasticLayer;
class MultiID;
class Muter;
class RayTracer1D;
class RayTracerRunner;
namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{


mClass AngleMuteBase 
{
public:
    mStruct Params
    {
			    Params()
				: mutecutoff_(30)
				, velvolmid_(MultiID::udf())
				{}	

	float 			mutecutoff_;
	MultiID			velvolmid_;   
	IOPar			raypar_;
    };

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }	
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

protected:
    			AngleMuteBase();
    			~AngleMuteBase();

    Params*		params_;

    Vel::VolumeFunctionSource*	velsource_;

    bool	setVelocityFunction();
    bool	getLayers(const BinID&,TypeSet<ElasticLayer>&,
	    			SamplingData<float>&,int resamplesz=-1);
    float	getOffsetMuteLayer(const RayTracer1D&,int,int,bool,
	    				int,bool) const;
    float	getOffsetMuteLayer(const RayTracer1D&,int,int,bool) const;
    void	getOffsetMuteLayers(const RayTracer1D&,int,int,bool,
	    				TypeSet< Interval<float> >&) const;

    ObjectSet<RayTracerRunner>	rtrunners_;
};



mClass AngleMute : public Processor, public AngleMuteBase
{
public:
    			mDefaultFactoryInstantiation(Processor,
				AngleMute,"AngleMute", "Angle Mute" );

			AngleMute();
			~AngleMute();

    mStruct AngleMutePars : public AngleMuteBase::Params
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
    bool		doWork(od_int64,od_int64,int);

    BufferString	errmsg_;
    bool		raytraceparallel_;
    Muter*		muter_;
};


}; //namespace

#endif

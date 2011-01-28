#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.3 2011-01-28 05:33:55 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"

class MultiID;
class Muter;
class RayTracer1D;
namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{


mClass AngleMute : public Processor
{
public:
    			mDefaultFactoryInstantiation(Processor,
				AngleMute,"AngleMute",sFactoryKeyword());

			AngleMute();
			~AngleMute();
    bool		prepareWork();
    bool		doPrepare(int nrthreads);

    			//Use this to set up parameters of the ray tracer
    RayTracer1D*	rayTracer()			{ return rtracer_; }
    MultiID		velocityVolumeID() const	{ return velvolmid_; }
    bool		setVelocityMid(const MultiID& mid);

    			//Muter setup
    bool		isTailMute() const		{ return tail_; }
    void		setTailMute(bool yn=true)	{ tail_ = yn; }
    float		taperLength() const		{ return taperlen_; }
    void		setTaperLength(float l)		{ taperlen_ = l; }
    void		setMuteCutoff(float co)		{ mutecutoff_ = co; }
    float		muteCutoff() const		{ return mutecutoff_; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }	
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

protected:

    od_int64 		nrIterations() const	{ return outputs_.size(); }
    bool		doWork(od_int64,od_int64,int);

    bool		raytraceparallel_;
    RayTracer1D*	rtracer_;
    MultiID		velvolmid_;   
    Muter*		muter_; 
    bool		tail_;
    float		taperlen_;
    float		mutecutoff_;
    Vel::VolumeFunctionSource*	velsource_;
};


}; //namespace

#endif

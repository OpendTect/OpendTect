#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.1 2011-01-26 23:10:42 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"

class MultiID;
class Muter;
class AngleRayTracer;
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
    AngleRayTracer*	angleTracer()			{ return rtracer_; }
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
    
    static const char*	sVelVolumeID()		{ return "Velocity vol-mid"; }
    static const char*  sMuteCutoff()		{ return "Mute cutoff"; }
    static const char*	sSourceDepth()		{ return "Source depth"; }
    static const char*	sReceiverDepth()	{ return "Receiver depth"; }

protected:

    od_int64 		nrIterations() const	{ return outputs_.size(); }
    bool		doWork(od_int64,od_int64,int);

    bool		raytraceparallel_;
    AngleRayTracer*	rtracer_;
    MultiID		velvolmid_;   
    Muter*		muter_; 
    bool		tail_;
    float		taperlen_;
    float		mutecutoff_;
    Vel::VolumeFunctionSource*	velsource_;
};


}; //namespace

#endif

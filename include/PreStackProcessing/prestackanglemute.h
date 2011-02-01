#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.5 2011-02-01 20:55:46 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "raytrace1d.h"


class MultiID;
class Muter;
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
    bool		doPrepare(int nrthreads);

    void		setSetup(const RayTracer1D::Setup&);
    const RayTracer1D::Setup& getSetup() const		{ return setup_; }
    MultiID		velocityVolumeID() const	{ return velvolmid_; }
    bool		setVelocityMid(const MultiID& mid);

    			//Muter setup
    void		setTailMute(bool yn=true);
    void		setTaperLength(float l);
    bool		isTailMute() const		{ return tail_; }
    float		taperLength() const		{ return taperlen_; }
    float		muteCutoff() const		{ return mutecutoff_; }
    void		setMuteCutoff(float co)		{ mutecutoff_ = co; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }	
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }

protected:

    od_int64 		nrIterations() const	{ return outputs_.size(); }
    bool		doWork(od_int64,od_int64,int);

    bool			raytraceparallel_;
    MultiID			velvolmid_;   
    Muter*			muter_; 
    bool			tail_;
    float			taperlen_;
    float			mutecutoff_;
    RayTracer1D::Setup		setup_;
    ObjectSet<RayTracer1D>	rtracers_;
    Vel::VolumeFunctionSource*	velsource_;
};


}; //namespace

#endif

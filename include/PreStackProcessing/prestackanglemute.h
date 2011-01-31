#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.4 2011-01-31 22:46:04 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "raytrace1d.h"


class MultiID;
class Muter;
class SeisTrcReader;
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
    bool		isTailMute() const		{ return tail_; }
    void		setTailMute(bool yn=true);
    float		taperLength() const		{ return taperlen_; }
    void		setTaperLength(float l);
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

    bool			raytraceparallel_;
    MultiID			velvolmid_;   
    Muter*			muter_; 
    bool			tail_;
    float			taperlen_;
    float			mutecutoff_;
    ObjectSet<RayTracer1D>	rtracers_;
    ObjectSet<SeisTrcReader>	velreaders_;
    Vel::VolumeFunctionSource*	velsource_;
    RayTracer1D::Setup		setup_;
};


}; //namespace

#endif

#ifndef prestackanglemute_h
#define prestackanglemute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		January 2011
 RCS:		$Id: prestackanglemute.h,v 1.9 2011-10-06 14:17:33 cvsbruno Exp $
________________________________________________________________________


-*/

#include "prestackprocessor.h"
#include "raytrace1d.h"
#include "samplingdata.h"


class MultiID;
class Muter;
namespace Vel { class VolumeFunctionSource; }

namespace PreStack
{


mClass AngleMuteBase 
{
public:
    mStruct Params
    {
			    Params()
				: mutecutoff_(0)
				, dovelblock_(false)
				, velvolmid_(MultiID::udf())
				{}	

	float 			mutecutoff_;
	bool			dovelblock_;
	MultiID			velvolmid_;   
	RayTracer1D::Setup	raysetup_;
    };

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyRayTracer()		{ return "Raytracer"; }	
    static const char*	sKeyVelVolumeID()	{ return "Velocity vol-mid"; }
    static const char*  sKeyMuteCutoff()	{ return "Mute cutoff"; }
    static const char*  sKeyVelBlock()		{ return "Block velocities"; }

protected:
    			AngleMuteBase();
    			~AngleMuteBase();

    Params*		params_;

    Vel::VolumeFunctionSource*	velsource_;

    bool	setVelocityFunction();
    bool	getLayers(const BinID&,TypeSet<ElasticLayer>&,
	    			SamplingData<float>&,int resamplesz=-1);
    float	getOffsetMuteLayer(const RayTracer1D&,int,int,bool) const;
};



mClass AngleMute : public Processor, public AngleMuteBase
{
public:
    			mDefaultFactoryInstantiation(Processor,
				AngleMute,"AngleMute",sFactoryKeyword());

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

    AngleMutePars&	 params();
    const AngleMutePars& params() const;

protected:

    od_int64 		nrIterations() const	{ return outputs_.size(); }
    bool		doWork(od_int64,od_int64,int);

    bool			raytraceparallel_;
    Muter*			muter_; 

    ObjectSet<RayTracer1D> 	rtracers_;
};


}; //namespace

#endif

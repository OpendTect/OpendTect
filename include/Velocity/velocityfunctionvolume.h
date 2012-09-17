#ifndef velocityfunctionvolume_h
#define velocityfunctionvolume_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunctionvolume.h,v 1.9 2011/09/20 14:16:48 cvsbruno Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"
#include "velocitycalc.h"


class BinIDValueSet;
class MultiID;
class SeisTrcReader;

namespace Vel
{

class VolumeFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Volume. */

mClass VolumeFunction : public Function
{
public:
			VolumeFunction(VolumeFunctionSource&);
    bool		moveTo(const BinID&);
    StepInterval<float>	getAvailableZ() const;
    StepInterval<float>	getLoadedZ() const;

    void		enableExtrapolation(bool);
    void		setStatics(float t,float vel);
    			//!<Only used with RMS velocities extrapolation

protected:

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;

    bool			zit_;
    SamplingData<float>		velsampling_;
    TypeSet<float>		vel_;

    bool			extrapolate_;
    float			statics_;
    float			staticsvel_;
};


mClass VolumeFunctionSource : public FunctionSource
{
public:
    mDefaultFactoryInstanciationBase( "Velocity volume", sFactoryKeyword() );
    				VolumeFunctionSource();
    const VelocityDesc&		getDesc() const 	{ return desc_; }

    bool			zIsTime() const;
    bool			setFrom(const MultiID& vel);

    VolumeFunction*		createFunction(const BinID&);

    void			getAvailablePositions(BinIDValueSet&) const;
    bool			getVel(const BinID&,SamplingData<float>&,
	    			       TypeSet<float>&);

    static const char*		sKeyZIsTime() { return "Z is Time"; }

protected:
    static FunctionSource* create(const MultiID&);
    				~VolumeFunctionSource();
   
    SeisTrcReader*		getReader();

    ObjectSet<SeisTrcReader>	velreader_;
    ObjectSet<const void>	threads_;

    Threads::Mutex		readerlock_;
    bool			zit_;
    VelocityDesc		desc_;
};


}; //namespace

#endif

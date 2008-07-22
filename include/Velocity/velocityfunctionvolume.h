#ifndef velocityfunctionvolume_h
#define velocityfunctionvolume_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunctionvolume.h,v 1.1 2008-07-22 17:39:21 cvskris Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"

class BinIDValueSet;
class MultiID;
class SeisTrcReader;

namespace Vel
{

class VolumeFunctionSource;


/*!VelocityFunction that gets its information from a Velocity Volume. */

class VolumeFunction : public Function
{
public:
			VolumeFunction(VolumeFunctionSource&);
    bool		moveTo(const BinID&);
    StepInterval<float>	getAvailableZ() const;

protected:

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;

    bool			zit_;
    SamplingData<float>		velsampling_;
    TypeSet<float>		vel_;

};


class VolumeFunctionSource : public FunctionSource
{
public:
    static void			initClass();
    				VolumeFunctionSource();
    static const char*		sType() { return "Velocity volume"; }
    const char*			type() const		{ return sType(); }

    const VelocityDesc&		getDesc() const 	{ return desc_; }

    bool			zIsTime() const;
    bool			setFrom(const MultiID& vel);

    VolumeFunction*		createFunction(const BinID&);

    void			getAvailablePositions(HorSampling&) const;
    bool			getVel(const BinID&,SamplingData<float>&,
	    			       TypeSet<float>&);

    static const char*		sKeyZIsTime() { return "Z is Time"; }

protected:
    static FunctionSource* create(const MultiID&);
    				~VolumeFunctionSource();
    
    SeisTrcReader*		velreader_;
    Threads::Mutex		readerlock_;
    bool			zit_;
    VelocityDesc		desc_;
};


}; //namespace

#endif

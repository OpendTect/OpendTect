#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"
#include "enums.h"

namespace Attrib
{

/*!
\brief %Instantaneous Attribute
*/

mExpClass(Attributes) Instantaneous : public Provider
{
public:
    enum OutType { Amplitude, Phase, Frequency, Hilbert, Amp1Deriv, Amp2Deriv,
		   CosPhase, EnvWPhase, EnvWFreq, PhaseAccel, ThinBed,
		   Bandwidth, QFactor, RotatePhase };
		   mDeclareEnumUtils(OutType);

    static void			initClass();
				Instantaneous(Desc&);

    static const char*		attribName()	{ return "Instantaneous"; }
    static const char*		rotateAngle()	{ return "rotationangle"; }

    void			getCompNames(BufferStringSet&) const;
    bool			prepPriorToOutputSetup();

protected:
				~Instantaneous() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int t0,int nrsamples,
					    int threadid) const;

    const Interval<int>*	reqZSampMargin(int,int) const;

    bool			allowParallelComputation() const
				{ return true; }

    bool			areAllOutputsEnabled() const;

    Interval<int>		sampgate1_;
    Interval<int>		sampgate2_;
    const DataHolder*		realdata_;
    const DataHolder*		imagdata_;
    int				realidx_;
    int				imagidx_;
    float			rotangle_;

private:
    float			calcAmplitude(int,int) const;
    float			calcPhase(int,int) const;
    float			calcFrequency(int,int) const;
    float			calcAmplitude1Der(int,int) const;
    float			calcAmplitude2Der(int,int) const;
    float			calcEnvWPhase(int,int) const;
    float			calcEnvWFreq(int,int) const;
    float			calcPhaseAccel(int,int) const;
    float			calcThinBed(int,int) const;
    float			calcBandWidth(int,int) const;
    float			calcQFactor(int,int) const;
    float			calcRotPhase(int,int,float angle) const;
    float			calcEnvWeighted(int,int,bool isphase) const;
};

}; // namespace Attrib


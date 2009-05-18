#ifndef instantattrib_h
#define instantattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.h,v 1.10 2009-05-18 10:33:38 cvshelene Exp $
________________________________________________________________________

-*/

/*! \brief
#### Short description
\par
#### Detailed description.

*/

#include "attribprovider.h"

namespace Attrib
{

mClass Instantaneous : public Provider
{
public:
    static void			initClass();
    				Instantaneous(Desc&);

    static const char*		attribName()	{ return "Instantaneous"; }

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

    Interval<int>		sampgate1_;
    Interval<int>		sampgate2_;
    const DataHolder*		realdata_;
    const DataHolder*		imagdata_;
    int				realidx_;
    int				imagidx_;

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
    float			calcRMSAmplitude(int,int) const;
};

}; // namespace Attrib

#endif

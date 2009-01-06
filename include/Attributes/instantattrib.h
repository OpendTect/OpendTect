#ifndef instantattrib_h
#define instantattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: instantattrib.h,v 1.9 2009-01-06 10:29:52 cvsranojay Exp $
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
    float			calcAmplitude(int) const;
    float			calcPhase(int) const;
    float			calcFrequency(int) const;
    float			calcAmplitude1Der(int) const;
    float			calcAmplitude2Der(int) const;
    float			calcEnvWPhase(int) const;
    float			calcEnvWFreq(int) const;
    float			calcPhaseAccel(int) const;
    float			calcThinBed(int) const;
    float			calcBandWidth(int) const;
    float			calcQFactor(int) const;
    float			calcRMSAmplitude(int) const;
};

}; // namespace Attrib

#endif

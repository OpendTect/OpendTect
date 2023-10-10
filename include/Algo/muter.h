#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "samplingdata.h"
template <class T> class ValueSeries;


/*!
\brief Sets start or end part of a float series to 0.

  This object measures distance in units of samples.

  Taper is cosine taper.
*/

mExpClass(Algo) Muter
{
public:
			Muter(float taperlen,bool tail=false);
			~Muter();

    void		mute(ValueSeries<float>&,int sz,float mutepos) const;
    void		muteIntervals(ValueSeries<float>&,int sz,
				      const TypeSet< Interval<float> >&) const;

    inline static float mutePos( float z, const SamplingData<double>& sd )
			{ return sd.getfIndex(z); }
    static void		muteIntervalsPos(const TypeSet< Interval<float> >&,
					TypeSet< Interval<float> >&,
					const SamplingData<double>&);

protected:

    float		taperlen_;
    bool		tail_;

    void		topMute(ValueSeries<float>&,int,float) const;
    void		tailMute(ValueSeries<float>&,int,float) const;
    void		itvMute(ValueSeries<float>&,int,Interval<float>) const;

};

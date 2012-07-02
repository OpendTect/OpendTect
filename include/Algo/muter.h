#ifndef muter_h
#define muter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: muter.h,v 1.6 2012-07-02 12:27:35 cvsbruno Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"

template <class T> class ValueSeries;

/*!\brief Sets start or end part of a float series to 0

  This object measures distance in units of samples.

  Taper is cosine taper.

  */

mClass Muter
{
public:
    			Muter( float taperlen, bool tail=false )
			    : taperlen_(taperlen)
			    , tail_(tail)		{}

    inline static float	mutePos( float z, const SamplingData<float>& sd )
			{ return sd.getfIndex(z); }

    void		mute(ValueSeries<float>&,int sz,float mutepos) const;


    static void 	muteIntervalsPos(const TypeSet< Interval<float> >&,
					TypeSet< Interval<float> >&,
					const SamplingData<float>&);
    void		muteIntervals(ValueSeries<float>&,int sz,
				      const TypeSet< Interval<float> >&) const;

protected:

    float		taperlen_;
    bool		tail_;

    void		topMute(ValueSeries<float>&,int,float) const;
    void		tailMute(ValueSeries<float>&,int,float) const;
    void		itvMute(ValueSeries<float>&,int,Interval<float>) const;

};

#endif

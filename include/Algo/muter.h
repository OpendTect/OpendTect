#ifndef muter_h
#define muter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: muter.h,v 1.2 2007-10-23 21:12:54 cvskris Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"

template <class T> class ValueSeries;

/*!\brief Sets start or end part of a float series to 0

  This object measures distance in units of samples.

  Taper is cosine taper.

  */

class Muter
{
public:
    			Muter( float taperlen, bool tail=false )
			    : taperlen_(taperlen)
			    , tail_(tail)		{}

    inline static float	mutePos( float z, const SamplingData<float>& sd )
			{ return (z - sd.start) / sd.step; }

    void		mute(ValueSeries<float>&,int sz,float mutepos) const;

protected:

    float		taperlen_;
    bool		tail_;

    void		topMute(ValueSeries<float>&,int,float) const;
    void		tailMute(ValueSeries<float>&,int,float) const;

};

#endif

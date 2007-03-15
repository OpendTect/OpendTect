#ifndef muter_h
#define muter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: muter.h,v 1.1 2007-03-15 20:02:40 cvskris Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"

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

    void		mute(float*,int sz,float mutepos) const;

protected:

    float		taperlen_;
    bool		tail_;

    void		topMute(float*,int,float) const;
    void		tailMute(float*,int,float) const;

};

#endif

#ifndef dataclipper_h
#define dataclipper_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		09-02-2002
 RCS:		$Id: dataclipper.h,v 1.6 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________


-*/

/*!\brief

A DataClipper gets a bunch of data and determines at what value to clip
if a certain clippercentag is desired.

Usage:
1. Create and set cliprate
2. putData() untill you have put all your data.
3. calculateRange
4. get the cliprange with getRange

Step 2-4 can be repeted any number of times.

*/

#include "sets.h"
#include "ranges.h"

class DataClipper
{
public:
    				DataClipper(float cliprate,float cliprate1=-1 );
				/*!< cliprate is between 0 and 0.5,
				     cliprate0 is the bottom cliprate,
				     cliprate1 is the top cliprate, when
				     cliprate1 is -1, it will get the value of
				     cliprate0 */

    void			setClipRate(float cr0,float cr1=-1);
    float			clipRate(bool bottom=true) const
				{ return bottom ? cliprate0 : cliprate1; }
	
    void			setApproxNrValues(int nrsamples,
						  int statsize=2000);
    				/*!< Will make it faster if large amount
				     (>10000 samples) of data is used. The
				     Object will then randomly subselect
				     on the input to get about statsize samples
				     to do the stats on.
				*/
    void			putData(float);
    void			putData(const float*, int );

    void			calculateRange();
    				/*!< Will also reset the stats so the
				     object becomes ready for new data
				*/

    const Interval<float>&	getRange() const { return range; }


protected:
    int				approxstatsize;
    float			sampleprob;
    bool			subselect;
    float			cliprate0;
    float			cliprate1;
    TypeSet<float>		samples;
    Interval<float>		range;
};


#endif

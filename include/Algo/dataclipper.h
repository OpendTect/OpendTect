#ifndef dataclipper_h
#define dataclipper_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		09-02-2002
 RCS:		$Id: dataclipper.h,v 1.4 2002-04-24 08:23:01 kristofer Exp $
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
    				DataClipper( float cliprate );
				/*!< cliprate is between 0 and 0.5 */
    void			setClipRate( float ncr ) { cliprate=ncr; }	
				/*!< cliprate is between 0 and 0.5 */
    float			clipRate() const { return cliprate; }
				/*!< cliprate is between 0 and 0.5 */

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
    float			cliprate;
    TypeSet<float>		samples;
    Interval<float>		range;
};


#endif

#ifndef fingervein_h
#define fingervein_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id: fingervein.h,v 1.3 2012-07-23 20:56:18 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "factory.h"

template <class T> class Array2D;

class TaskRunner;

/*Get a flag output for faults */

mClass FingerVein
{
public:    

    				FingerVein(const Array2D<float>&,
					   float threshold,bool isabove,
					   bool istimeslcie,
					   Array2D<bool>& output);
				~FingerVein()	{}

    bool			compute(bool domerge=false,int minfltlength=15,
	    				float overlaprate=0.1,TaskRunner* tr=0); 

protected:

    bool			computeMaxCurvature(Array2D<float>&,int sigma,
	    					    TaskRunner* tr);
    void			thinning(Array2D<bool>& res);
    void			thinStep(const Array2D<bool>& input,
					 Array2D<bool>& output,bool isfirst);
    void			removeSmallComponents(Array2D<bool>&,int minfltlength,
						      float overlaprate);

    const Array2D<float>&	input_;
    Array2D<bool>&		output_;
    float			threshold_;
    bool			isabove_;
    bool			istimeslice_;
};



#endif

#ifndef fingervein_h
#define fingervein_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id: fingervein.h,v 1.1 2012-07-13 20:13:57 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "factory.h"
#include "odmemory.h"

template <class T> class Array2D;
template <class T> class Array3D;

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

    bool			compute(TaskRunner* tr=0); 

protected:

    bool			computeMaxCurvature(Array2D<float>&,int sigma,
	    					    TaskRunner* tr);
    void			thinning(Array2D<bool>& res);
    void			condition(const Array2D<bool>& input,
	    				   Array2D<bool>& output,bool isfirst);

    const Array2D<float>&	input_;
    Array2D<bool>&		output_;
    float			threshold_;
    bool			isabove_;
    bool			istimeslice_;
};



#endif

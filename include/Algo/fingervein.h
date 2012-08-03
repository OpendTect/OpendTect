#ifndef fingervein_h
#define fingervein_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id: fingervein.h,v 1.5 2012-08-03 13:00:03 cvskris Exp $
________________________________________________________________________


-*/

#include "algomod.h"
#include "factory.h"

template <class T> class Array2D;

class TaskRunner;

/*Get a flag output for faults */

mClass(Algo) FingerVein
{
public:    

    				FingerVein(const Array2D<float>&,
					   float threshold,bool isabove,
					   bool istimeslcie,
					   Array2D<bool>& output);
				~FingerVein()	{}

    bool			compute(bool domerge=true,bool dothinning=true,
	    				int minfltlength=15,
					float overlaprate=0.5,TaskRunner* tr=0);
    const TypeSet<TypeSet<int> >& validConnComponents() const 
    				{ return validconncomps_; }
    const TypeSet<int>& 	nrConnComponents() const { return nrcomps_; }
    const TypeSet<int>& 	compIndices() const  	 { return compids_; }
protected:

    bool			computeMaxCurvature(Array2D<float>&,int sigma,
	    					    TaskRunner* tr);
    void			thinning(Array2D<bool>& res);
    void			thinStep(const Array2D<bool>& input,
					 Array2D<bool>& output,bool isfirst);
    void			removeSmallComponents(Array2D<bool>&,
	    				int minfltlength,float overlaprate,
					bool savecomps=true);

    const Array2D<float>&	input_;
    Array2D<bool>&		output_;
    float			threshold_;
    bool			isabove_;
    bool			istimeslice_;
    TypeSet<TypeSet<int> >	validconncomps_;
    TypeSet<int>		nrcomps_;
    TypeSet<int>		compids_;
};



#endif


#ifndef faultextractor_h
#define faultextractor_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng LIu
 Date:		February 2012
 RCS:		$Id: faultextractor.h,v 1.1 2012/02/14 23:20:31 cvsyuancheng Exp $
________________________________________________________________________

*/


#include "ranges.h"

namespace EM { class Fault3D; }
template <class T> class Array3D;
class Coord3;
class CubeSampling;

/*!\brief
Based on Hough transform, find faults in Array3D<float>'s with threshold. 
*/

mClass FaultExtractor
{
public:
    				FaultExtractor(const Array3D<float>&,
					       const CubeSampling&);
    				~FaultExtractor();

    bool			compute();

    void			setThreshold(float val,bool above_val);
    				/*<Make hough array based on above/below val.*/
    void			setLineAngleRange(Interval<float>rg);
    				/*<rg is between 0-PI in radian.*/
    void			setTopList(int nr)	{ toplistnr_ = nr; }
    				/*<nr is the number of top confident lines.*/
    ObjectSet<EM::Fault3D>	getFaults() const	{ return faults_; }

protected:

    void			makeFaultStick(int,int,float,float,
	    					EM::Fault3D&,int);

    const Array3D<float>&	input_;

    Array3D<int>*		harr0_;	
    Array3D<int>*		harr1_;
    Array3D<unsigned char>*	flag_;

    float			threshold_;
    bool			abovethreshold_;
    int				toplistnr_; 
    Interval<float>		anglerg_;

    const CubeSampling&		cs_;
    ObjectSet<EM::Fault3D>	faults_;
};


#endif

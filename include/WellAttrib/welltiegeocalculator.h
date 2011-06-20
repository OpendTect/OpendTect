#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.25 2011-06-20 11:55:52 cvsbruno Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

/* !brief performs the computations needed by TWTS  !*/   

namespace Well { class D2TModel; class Track; class Log; }
template <class T> class Array1DImpl;

namespace WellTie
{

mClass GeoCalculator
{
public :
//Well data operations
    Well::D2TModel* 	getModelFromVelLog(const Well::Log&,
					   const Well::Track*,
					   float surfelev, bool issonic) const;
    void		ensureValidD2TModel(Well::D2TModel&) const;

    enum		Conv { Vel2TWT, Son2TWT, TWT2Vel, Son2Vel, Vel2Son };
    void 		velLogConv(Well::Log&,Conv) const;
    void		d2TModel2Log(const Well::D2TModel&,Well::Log&) const;

//others  
    void		removeSpikes(float* inp,int sz,int gate,int fac) const;
    int 		getIdx(const Array1DImpl<float>&,float) const; 
    double 		crossCorr(const float*,const float*,float*,int) const;
    void 		deconvolve(const float*,const float*,float*,int) const;
};

}; //namespace WellTie
#endif

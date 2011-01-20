#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.22 2011-01-20 10:21:38 cvsbruno Exp $
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
					   float surfelev) const;
    void		ensureValidD2TModel(Well::D2TModel&) const;

    enum		Conv { Vel2TWT, Son2TWT, TWT2Vel, Son2Vel, Vel2Son };
    void 		velLogConv(Well::Log&,Conv) const;

//stretch/squeeze
    mStruct StretchData
    {			StretchData()
			    : inp_(0)
			    , outp_(0)
			    , stretchfac_(0)
			    , squeezefac_(0)
			    , isstretch_(true)
			    {}

	const Array1DImpl<float>* inp_;
	Array1DImpl<float>* outp_;
	int 		start_, stop_, pick1_, pick2_;

	bool		isstretch_;
	float		stretchfac_;
	float		squeezefac_;
    };
    void		stretch(StretchData&) const;
    void 		stretch(const StretchData&,float) const;

//others  
    void		removeSpikes(float* vals,int sz, int gatesz) const;
    int 		getIdx(const Array1DImpl<float>&,float) const; 
    double 		crossCorr(const float*,const float*,float*,int) const;
    void 		deconvolve(const float*,const float*,float*,int) const;
};

}; //namespace WellTie
#endif

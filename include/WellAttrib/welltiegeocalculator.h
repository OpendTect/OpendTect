#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.30 2012-08-03 13:00:47 cvskris Exp $
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "commondefs.h"

/* !brief performs the computations needed by TWTS  !*/   

namespace Well { class D2TModel; class Track; class Log; class Data; }
template <class T> class Array1DImpl;

namespace WellTie
{

mClass(WellAttrib) GeoCalculator
{
public :
//Well data operations
    Well::D2TModel* 	getModelFromVelLog(const Well::Data&,const char* son, 
					   bool issonic) const;
    void		ensureValidD2TModel(Well::D2TModel&,const Well::Data&,
					    float) const;

    void		son2TWT(Well::Log&,bool straight,float startdah) const;
    void 		vel2TWT(Well::Log&,bool straight,float startdah) const;
    void		son2Vel(Well::Log&,bool yn) const;
    void		d2TModel2Log(const Well::D2TModel&,Well::Log&) const;
    float		getSRDElevation(const Well::Data&) const;

//others  
    void		removeSpikes(float* inp,int sz,int gate,int fac) const;
    double 		crossCorr(const float*,const float*,float*,int) const;
    void 		deconvolve(const float*,const float*,float*,int) const;
};

}; //namespace WellTie
#endif


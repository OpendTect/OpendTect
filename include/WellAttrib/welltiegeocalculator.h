#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "commondefs.h"
#include "odcomplex.h"

/* !brief performs the computations needed by TWTS  !*/   

namespace Well { class D2TModel; class Track; class Log; class Data; }
template <class T> class Array1DImpl;

namespace WellTie
{

mClass GeoCalculator
{
public :
//Well data operations
    Well::D2TModel* 	getModelFromVelLog(const Well::Data&,const char* son, 
					   bool issonic,float replacevel) const;
			// do not use, will be removed
    void		ensureValidD2TModel(Well::D2TModel&,
	    				    const Well::Data&) const;
    void		son2TWT(Well::Log&,bool straight,float startdah) const;
    			// do not use, will be removed
    void 		vel2TWT(Well::Log&,bool straight,float startdah) const;
			// do not use, will be removed
    void		son2Vel(Well::Log&,bool yn) const;
			// do not use, will be removed
    void		d2TModel2Log(const Well::D2TModel&,Well::Log&) const;
    float		getSRDElevation(const Well::Data&) const;
			// do not use, will be removed

//others  
    void		removeSpikes(float* inp,int sz,int gate,int fac) const;
    double 		crossCorr(const float*,const float*,float*,int) const;
    void 		deconvolve(const float*,const float*,float*,int) const;
			// do not use, will be removed


public:
    void		ensureValidD2TModel(Well::D2TModel&,const Well::Data&,
					    float replacevel ) const;
			// do not use, will be removed
    Well::D2TModel* 	getModelFromVelLog(const Well::Data&,const char* son, 
					   bool issonic) const;
			// do not use, will be removed
    void		son2TWT(Well::Log&,const Well::Track&,bool,float) const;
			// do not use, will be removed
    void		vel2TWT(Well::Log&,const Well::Track&,bool,float) const;
			// do not use, will be removed

public:
    Well::D2TModel* 	getModelFromVelLog(const Well::Data&,
	    				   const char* lognm) const;
    void		son2TWT(Well::Log&,const Well::Data&) const;
    void		vel2TWT(Well::Log&,const Well::Data&) const;
    void		son2Vel(Well::Log&) const;
    void 		deconvolve(const float*,const float_complex*,float*,
	    			   int) const;
};

}; //namespace WellTie
#endif


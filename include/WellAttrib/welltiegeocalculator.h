#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.10 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "arrayndimpl.h"
#include "fft.h"

#include <complex>

/*
  brief class to perform the computations needed by TWTS  
*/   

class WellTieSetup;
class Wavelet;
class WellTieParams;
class IOPar;
namespace Well
{
    class MGR; 
    class Data;
    class D2TModel;
}


mClass WellTieGeoCalculator
{
public:
			WellTieGeoCalculator(const WellTieParams*,
					     const Well::Data*);
			~WellTieGeoCalculator() {};

//d2tm operations
    Well::D2TModel* 	getModelFromVelLog(const char*, bool);
    Well::D2TModel*     getModelFromVelLogData(const Array1DImpl<float>&,
	                                       const Array1DImpl<float>&);
    void		setVelLogDataFromModel(const Array1DImpl<float>&,    
						const Array1DImpl<float>&,	    						Array1DImpl<float>&);
    void 		TWT2Vel(const TypeSet<float>&,const TypeSet<float>&,
				TypeSet<float>&,bool);
//logs operations
    void                computeAI(const Array1DImpl<float>&,
				 const Array1DImpl<float>&,Array1DImpl<float>&);
    void                computeReflectivity(const Array1DImpl<float>&,
					    Array1DImpl<float>&,int);
    void 		interpolAtIdx(float,float,float,float&);
    void 		interpolateLogData(TypeSet<float>&,float,bool);
    bool 		isValidLogData(const TypeSet<float>&);

//wvlt operations
    void                convolveWavelet(const Array1DImpl<float>&,
	    				const Array1DImpl<float>&, 
					Array1DImpl<float>&,int);
    void 		deconvolve( const Array1DImpl<float>&,
				    const Array1DImpl<float>&,
				    Array1DImpl<float>&,int);
    void 		reverseWavelet(Wavelet&);

//other operations    
    void 		crosscorr( const Array1DImpl<float>&,
				  const Array1DImpl<float>&,
				  Array1DImpl<float>&);
    int 		getFirstDefIdx(const TypeSet<float>&);
    int 		getLastDefIdx(const TypeSet<float>&);
    void 		lowPassFilter(Array1DImpl<float>&,float);
    void		resampleData(const Array1DImpl<float>&,
				     Array1DImpl<float>&,float);
    void		stretchArr(const Array1DImpl<float>&,
				     Array1DImpl<float>&,int,int,int,int);
    void		zeroPadd(const Array1DImpl<float_complex>&,
				       Array1DImpl<float_complex>&);
    int 		getIdx(const Array1DImpl<float>&, float); 


protected:

    const Well::Data&	wd_;
    const WellTieSetup& wtsetup_;
    const WellTieParams& params_;
    const double 	denfactor_;
    const double 	velfactor_;
};

#endif

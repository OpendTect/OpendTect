#ifndef welltiegeocalculator_h
#define welltiegeocalculator_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Apr 2009
 RCS:           $Id: welltiegeocalculator.h,v 1.4 2009-05-15 12:42:48 cvsbruno Exp $
________________________________________________________________________

-*/


/*
  brief class to perform the computations needed by TWTS  
*/   
#include "namedobj.h"

template <class T> class Array1DImpl;
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


    Well::D2TModel* 	getModelFromVelLog(bool);
    Well::D2TModel*     getModelFromVelLogData(const Array1DImpl<float>&,
	                                       const Array1DImpl<float>&);
    void		setVelLogDataFromModel(const Array1DImpl<float>&,    
						const Array1DImpl<float>&,	    						Array1DImpl<float>&);
    int 		getFirstDefIdx(const TypeSet<float>&);
    int 		getLastDefIdx(const TypeSet<float>&);
    void 		interpolateLogData(TypeSet<float>&,float,bool);
    bool 		isValidLogData(const TypeSet<float>&);
    void 		TWT2Vel(const TypeSet<float>&,const TypeSet<float>&,
				TypeSet<float>&,bool);
    void 		lowPassFilter(Array1DImpl<float>&,float);
    void		stretchArr(const Array1DImpl<float>&,
				     Array1DImpl<float>&,int,int,int,int);
    void 		interpolAtIdx(float,float,float,float&);
    void		resampleData(const Array1DImpl<float>&,
				     Array1DImpl<float>&,float);
    void                computeAI(const Array1DImpl<float>&,
				 const Array1DImpl<float>&,Array1DImpl<float>&);
    void                computeReflectivity(const Array1DImpl<float>&,
					    Array1DImpl<float>&,int);
    void                convolveWavelet(const Array1DImpl<float>&,
	    				const Array1DImpl<float>&, 
					Array1DImpl<float>&,int);
    void 		reverseWavelet(Wavelet&);
    void 		deconvolve( const Array1DImpl<float>&,
				    const Array1DImpl<float>&,
				    Array1DImpl<float>&,int);

protected:

    const Well::Data&	wd_;
    const WellTieSetup& wtsetup_;
    const WellTieParams& params_;
    const double 	denfactor_;
    const double 	velfactor_;
};

#endif

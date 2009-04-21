#ifndef welltieunitfactors_h
#define welltieunitfactors_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieunitfactors.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/


#include "namedobj.h"

class UnitOfMeasure;
class WellTieSetup;
namespace Well
{
    class Data;
}

mClass WellTieUnitFactors
{
public:

		    WellTieUnitFactors(const WellTieSetup*);
		    ~WellTieUnitFactors() {};

   
    const double 	velFactor() const          { return velfactor_; }
    const double 	denFactor() const          { return denfactor_; }

    const UnitOfMeasure*  velUOM() const 	   { return veluom_;}	
    const UnitOfMeasure*  denUOM() const	   { return denuom_;}	

protected:
 
    const UnitOfMeasure*  veluom_;
    const UnitOfMeasure*  denuom_;
    double	 	velfactor_;
    double 	 	denfactor_;

    void	 	calcVelFactor(const char*,bool);
    void 	 	calcVelFactor(const char*);
    void 	 	calcSonicVelFactor(const char*);    
    void  	 	calcDensFactor(const char*);
};


#endif

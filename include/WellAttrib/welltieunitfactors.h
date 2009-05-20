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
#include "ranges.h"
#include "bufstringset.h"
#include "welltiesetup.h"


class UnitOfMeasure;
namespace Attrib { class DescSet; }
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



/*!\collects the parameters used for TWTS. */
mClass WellTieParams
{
public :
				WellTieParams(const WellTieSetup&,
					      Well::Data*,
					      const Attrib::DescSet&);
				~WellTieParams(){};

    const WellTieSetup& 	getSetup() const   { return wtsetup_; }	 
    const WellTieUnitFactors& 	getUnits() const   { return factors_; }
    const StepInterval<float>&  getTimeScale() const  { return timeintv_; } 

    BufferStringSet		colnms_;
    const int 			nrdatacols_;				    
    float 			startdah_;				    
    float 			stopdah_;				    
    StepInterval<float> 	timeintv_;
    int 			step_;
    int           		worksize_;
    int           		dispsize_;
    const char*			currvellognm_;
    const char*			ainm_;
    const char*			refnm_;
    BufferString		attrnm_;
    const char*			timenm_;
    const char*			dptnm_;
    const char*			synthnm_;
    bool 			iscsavailable_;
    bool                        iscscorr_;
    bool                        iscsdisp_;
    const Attrib::DescSet& 	ads_;

    bool			resetTimeParams(CallBacker*);
    BufferString	 	getAttrName(const Attrib::DescSet&) const;

protected :

    Well::Data&			wd_;
    WellTieSetup		wtsetup_;
    const WellTieUnitFactors    factors_;
    
    void	 		createColNames();
};

#endif

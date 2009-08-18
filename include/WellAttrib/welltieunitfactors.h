#ifndef welltieunitfactors_h
#define welltieunitfactors_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieunitfactors.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/


#include "namedobj.h"
#include "ranges.h"
#include "bufstringset.h"
#include "welldata.h"

class UnitOfMeasure;
class WellTieSetup;
namespace Attrib { class DescSet; }


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

    double	 	calcVelFactor(const char*,bool);
    double 	 	calcVelFactor(const char*);
    double 	 	calcSonicVelFactor(const char*);    
    double  	 	calcDensFactor(const char*);
};



/*!\collects the parameters used for TWTS, linked to ui and data computation*/
mClass WellTieParams
{
public :
				WellTieParams(const WellTieSetup&,
					      Well::Data*,
					      const Attrib::DescSet&);
				~WellTieParams(){};

    mStruct uiParams
    {
			    uiParams(const Well::Data*);

	bool                    iscsavailable_;
	bool                    iscscorr_;
	bool 			iscsdisp_;
	bool                    ismarkerdisp_;
	bool                    iszinft_;
	bool                    iszintime_;
	const Well::Data&	wd_;
	
	friend class 		WellTieParams;
    };

    mStruct DataParams
    {
			   DataParams(const Well::Data* d,const WellTieSetup& w)
				: wd_(*d)
				, wts_(w)  
				, nrdatacols_(10)
				, step_(20)
				, timeintv_(0,0,0)
				, dispsize_(0)  
				, corrsize_(0)  
				, worksize_(0) 
				{}

	int           		worksize_;
	int           		dispsize_;
	int           		corrsize_;
	int           		nrdatacols_;
	StepInterval<double> 	timeintv_;
	StepInterval<double> 	dptintv_;
	StepInterval<double> 	corrtimeintv_;
	float			corrstartdah_;
	float			corrstopdah_;
	int 			step_;
	BufferStringSet		colnms_;
	BufferString		denlognm_;
	BufferString		vellognm_;
	BufferString		corrvellognm_;
	BufferString		currvellognm_;
	BufferString		dispcurrvellognm_;
	BufferString		ainm_;
	BufferString		refnm_;
	BufferString		attrnm_;
	BufferString		timenm_;
	BufferString		dptnm_;
	BufferString		synthnm_;
	BufferString		crosscorrnm_;
    
	const WellTieSetup&	wts_;
	const Well::Data&	wd_;
	void	 		createColNames();
	bool			resetDataParams();
	bool			setTimes(StepInterval<double>&,float,float);
	bool			setDepths(const StepInterval<double>&,
						StepInterval<double>&);
    	const StepInterval<double>&  getTimeScale() const { return timeintv_; } 

	friend class WellTieParams;
    };

    uiParams			uipms_;
    DataParams			dpms_;

    const Attrib::DescSet& 	ads_;

    const WellTieSetup& 	getSetup() const   { return wtsetup_; }
    const WellTieUnitFactors& 	getUnits() const   { return factors_; }
    BufferString	 	getAttrName(const Attrib::DescSet&) const;
    bool			resetParams(); 
    void			resetVellLognm();

protected :

    const WellTieSetup&		wtsetup_;
    Well::Data&			wd_;
    const WellTieUnitFactors    factors_;
};

#endif

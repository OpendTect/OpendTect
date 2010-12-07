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
#include "multiid.h"
#include "ranges.h"
#include "bufstringset.h"
#include "welldata.h"

class UnitOfMeasure;
class CubeSampling;

namespace WellTie
{
    class Setup;

mClass UnitFactors
{
public:

		    UnitFactors(const WellTie::Setup*);
		    ~UnitFactors() {};

   
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
mClass Params
{
public :
				Params(const WellTie::Setup&,
				       const Well::Data*);
				~Params(){};

    mStruct uiParams
    {
			    uiParams(const Well::Data* d)
				: wd_(d)
				, iscsdisp_(false)
				, ismarkerdisp_(d->haveMarkers())
				, isvwrmarkerdisp_(d->haveMarkers())
				, isvwrhordisp_(false)
				, iszinft_(false)
				, iszintime_(true)
				, dispmrkfullnames_(false)
				, disphorfullnames_(false)
				{}

	bool                    iscsavailable_;
	bool                    iscscorr_;
	bool 			iscsdisp_;
	bool                    ismarkerdisp_;
	bool                    isvwrmarkerdisp_;
	bool                    isvwrhordisp_;
	bool                    dispmrkfullnames_;
	bool                    disphorfullnames_;
	bool                    iszinft_;
	bool                    iszintime_;

	void			resetWD(const Well::Data* wd)
	    			{ wd_ = wd; }
	const Well::Data*	wd_;
    };

    mStruct DataParams
    {
			   DataParams(const Well::Data* d,
				      const WellTie::Setup& w)
				: wd_(d)
				, wts_(w)  
				, step_(20)
				, isinitwvltactive_(true) 	  
				, estwvltlength_(0) 	  
				, cs_(0)
	      			, extractseismic_(true)	
				{}

	TypeSet< StepInterval<float> > timeintvs_;
	int 			step_;

	BufferStringSet		colnms_;
	BufferString		denlognm_;
	BufferString		vellognm_;
	BufferString		corrvellognm_;
	BufferString		currvellognm_;
	BufferString		dispcurrvellognm_;
	BufferString		ainm_;
	BufferString		refnm_;
	BufferString		seisnm_;
	BufferString		timenm_;
	BufferString		dptnm_;
	BufferString		synthnm_;
	BufferString		crosscorrnm_;

	bool			isinitwvltactive_;
	bool			extractseismic_;
	int			estwvltlength_;
    
	CubeSampling*		cs_;
    
	const WellTie::Setup&	wts_;
	const Well::Data*	wd_;
	void			resetWD(const Well::Data* wd)
	    			{ wd_ = wd; }
	float			d2T(float,bool istime = true) const;
	Interval<float>		d2T(Interval<float>,bool time = true) const;
	void	 		createColNames();
	bool			resetTimeParams();
	void			setUpCubeSampling();
	const CubeSampling*	getCubeSampling() const { return cs_; } 
    };

    uiParams			uipms_;
    DataParams			dpms_;

    bool			resetParams() {return dpms_.resetTimeParams();} 
    void			resetVelLogNm();
    void			resetWD(const Well::Data*);

protected :

    const WellTie::Setup&	wtsetup_;
    const Well::Data*		wd_;
};

};//namespace WellTie

#endif

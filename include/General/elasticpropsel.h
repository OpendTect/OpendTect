#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.2 2011-07-20 14:23:49 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "ailayer.h"
#include "commondefs.h"
#include "enums.h"
#include "undefval.h"

mStruct ElasticProps
{
		    ElasticProps( float& den, float& pvel, float& svel )
			: den_(den)
			, pvel_(pvel)
			, svel_(svel)
			, ai_(mUdf(float))	     
			{}

    float pvel_, svel_, den_, ai_;
};


mClass ElasticPropSel
{
public:

    enum valStatus 	{ FromVel, FromAI };
    DeclareEnumUtils( valStatus );

    mStruct Params	
    {
	valStatus 	pvelstat_, svelstat_, denstat_; 
	float		pvel2svelafac_;
	float		pvel2svelbfac_;
    };

    			ElasticPropSel(const Params& par) 
			    : params_(par)
			    {}

    void		fill(AILayer&);
    void		fill(ElasticProps&);
    void		fill(ElasticLayer&);
    void		fill(AIModel&);
    void		fill(ElasticModel&);

protected:

    void		fillPVel(ElasticProps&);
    void		fillSVel(ElasticProps&);
    void		fillDen(ElasticProps&);

    const Params& 	params_;
};

#endif


#ifndef elasticpropsel_h
#define elasticpropsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		May 2011
 RCS:		$Id: elasticpropsel.h,v 1.1 2011-05-17 08:09:27 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief assigns values to an elastic layer depending on user defined parameters !*/

#include "commondefs.h"
#include "enums.h"
#include "undefval.h"

mStruct ElasticProps
{
		    ElasticProps()
			: pvel_(mUdf(float))
			, svel_(mUdf(float))
			, den_(mUdf(float))
			, ai_(mUdf(float))
			{}

    float pvel_, svel_, den_, ai_;
};


mClass ElasticPropSel
{
public:

    enum valStatus 	{ Assigned, Constant, FromVel, FromAI };
    DeclareEnumUtils( valStatus );

    mStruct Params	
    {
	valStatus 	pvelstat_, svelstat_, denstat_; 
    };

    			ElasticPropSel(const Params& par) 
			    : params_(par)
			    {}

    void		fill(ElasticProps&);

protected:

    void		fillPVel(ElasticProps&);
    void		fillSVel(ElasticProps&);
    void		fillDen(ElasticProps&);

    const Params& 	params_;
};

#endif


#ifndef flatposdata_h
#define flatposdata_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2007
 RCS:           $Id: flatposdata.h,v 1.1 2007-02-19 16:43:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "ranges.h"
#include "indexinfo.h"

/*!\brief Positioning of flat 'bulk' data.
  	  Only the 'x1' axis can be irregular. */

class FlatPosData
{
public:
				FlatPosData()
				    : x1rg_(0,0,1)
				    , x2rg_(0,0,1)
				    , x1pos_(0)
    				    , x1offs_(0)	{}
				~FlatPosData()		{ delete x1pos_; }

    void			setRange(bool forx1,
	    				 const StepInterval<double>&);
    StepInterval<double>	range( bool forx1 ) const
				{ return forx1 ? x1rg_ : x2rg_; }
    void			setX1Pos(const float*,int sz,double offs);
    				//!< offs is added to each elem before display
    				//!< This makes sure float* can be used
    float*			getPositions(bool forx1) const;
    				//!< Returns a new float [], size=nrPts()

    int				nrPts(bool forx1) const;
    float			width(bool forx1) const;
    IndexInfo			indexInfo(bool forx1,double) const;

protected:

    StepInterval<double>	x1rg_;
    StepInterval<double>	x2rg_;

    TypeSet<float>*		x1pos_; //!< used only if !null and size() > 0
    double			x1offs_;
};


#endif

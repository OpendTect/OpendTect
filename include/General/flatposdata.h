#ifndef flatposdata_h
#define flatposdata_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "ranges.h"
#include "indexinfo.h"

/*!\brief Positioning of flat 'bulk' data.
  	  Only the 'x1' axis can be irregular.

    Even if the X1 range is irergular, the x1rg_ is kept in sync with the
    start, stop and average step. Uncomplicated programmers may get away
    with using nothing else but the range().
 
 */

mExpClass(General) FlatPosData
{
public:
				FlatPosData()
				    : x1rg_(0,0,1)
				    , x2rg_(0,0,1)
				    , x1pos_(0)
    				    , x1offs_(0)	{}
				FlatPosData( const FlatPosData& fpd )
				    : x1pos_(0)		{ *this = fpd; }
				~FlatPosData()		{ delete [] x1pos_; }
    FlatPosData&		operator =(const FlatPosData&);
    FlatPosData*		clone() { return new FlatPosData(*this); }

    void			setRange(bool forx1,
	    				 const StepInterval<double>&);
    inline const StepInterval<double>& range( bool forx1 ) const
				{ return forx1 ? x1rg_ : x2rg_; }
    void			setX1Pos(float*,int sz,double offs);
    				//!< offs is added to each elem before display
    				//!< This makes sure float* can be used
    				//!< float* (alloc with new []) becomes mine

    inline int			nrPts( bool forx1 ) const
				{ return range(forx1).nrSteps() + 1; }
    float			width( bool forx1 ) const
				{ return ( float )( range(forx1).width() ); }
    IndexInfo			indexInfo(bool forx1,double pos) const;

    double			position(bool forx1,int) const;
    				//!< With offset applied
    void			getPositions(bool forx1,TypeSet<float>&) const;
    				//!< With offset applied
    float*			getPositions(bool forx1) const;
    				//!< Returns a new float [], size=nrPts()
    				//!< offset not applied (it's a float*)
    inline double		offset( bool forx1 ) const
    				{ return x1offs_ ? x1offs_ : 0; }
    				//!< For use with getPositions

    inline bool			isIrregular() const
				{ return x1pos_ && nrPts(true) > 2; }
    inline void			setRegular()
				{ setRange( true, range(true) ); }

protected:

    StepInterval<double>	x1rg_;
    StepInterval<double>	x2rg_;

    float*			x1pos_;
    double			x1offs_;

    inline StepInterval<double>& rg( bool forx1 )
				{ return forx1 ? x1rg_ : x2rg_; }
};


#endif


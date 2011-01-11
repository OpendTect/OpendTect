#ifndef wellhorpos_h
#define wellhorpos_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jul 2010
 RCS:           $Id: wellhorpos.h,v 1.6 2011-01-11 10:47:33 cvsbruno Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "namedobj.h"
#include "position.h"

/*!brief used to give well info at horizon intersection. !*/

namespace Well { class Track; class D2TModel; }
class BinIDValueSet;
class MultiID;

mClass WellHorIntersectFinder
{
public:
    				WellHorIntersectFinder(const Well::Track&,
						const Well::D2TModel* d2t=0);
				//! a d2t model is needed if z is time 
				//and the track is not 

    void			setHorizon(const EM::ObjectID& emid)
				{ horid_ = emid; }

    mStruct ZPoint
    {
				ZPoint( BinID b, float z )
				    : bid_(b), zval_(z) {}

	 bool            	operator ==( const ZPoint& pt ) const
					     { return bid_ == pt.bid_; }

	BinID			bid_;
	float			zval_;
    };
    void			findIntersection(TypeSet<ZPoint>&) const;
    				//get pos at Well track/Horizon intersection. 
    				//In principle, only one position per well but 
    				//the horizon can intersect the track more 
    				//than once ( faults, deviated track )
protected:

    TypeSet<ZPoint>		wellpts_;
    EM::ObjectID 	 	horid_;

    void			intersectBinIDHor(const BinID&,float&) const;
    void			findIntersect(const TypeSet<ZPoint>&,
						     TypeSet<ZPoint>&) const;
    void 			transformWellCoordsToBinIDs(const Well::Track&);
};

#endif

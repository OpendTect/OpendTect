#ifndef faulthorintersect_h
#define faulthorintersect_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          March 2010
RCS:           $Id: faulthorintersect.h,v 1.1 2010-03-15 19:24:57 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "task.h"

class Coord3List;

namespace Geometry
{

class BinIDSurface;
class FaultStickSet;
class IndexedShape;


mClass FaultBinIDSurfaceIntersector
{
public:
			FaultBinIDSurfaceIntersector(float horshift,
						     const BinIDSurface&,
						     const FaultStickSet&,
						     Coord3List&);
			~FaultBinIDSurfaceIntersector();

    void		compute();		

    void		setShape(const IndexedShape&);
    const IndexedShape*	getShape(bool takeover=true);

protected:

    void		doPrepare();
    void		addIntersectionsToList();    
    void		computeStickIntersectionInfo(int stickidx);

    struct IntSectInfo
    {
	int	lowknotidx;
	Coord3	intsectpos;
    };
    
    ObjectSet< TypeSet<BinID> >		ftbids_;
    ObjectSet< Interval<float> >	zrgs_;
    ObjectSet< IntSectInfo >		stickitsinfo_;    

    const BinIDSurface&			surf_;
    const FaultStickSet&		ft_;
    Coord3List&				crdlist_;
    const IndexedShape*			output_;
    float				zshift_;
};


};


#endif

#ifndef faulthorintersect_h
#define faulthorintersect_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          March 2010
RCS:           $Id: faulthorintersect.h,v 1.3 2011-02-07 22:58:10 cvsyuancheng Exp $
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

    void		calPanelIntersections(int panelidx,TypeSet<Coord3>&);
    friend class	FaultStickHorizonIntersector;

    struct StickIntersectionInfo
    {
	int	lowknotidx;
	Coord3	intsectpos;
	char	intersectstatus;
		/*-1 = all stick knots below the horizon, 
		   0 = intersect with horizon,
		   1 = all stick knots above the horizon. */
    };
    
    ObjectSet< TypeSet<BinID> >		ftbids_;
    ObjectSet< StickIntersectionInfo >	itsinfo_;   

    const BinIDSurface&			surf_;
    const FaultStickSet&		ft_;
    Coord3List&				crdlist_;
    const IndexedShape*			output_;
    float				zshift_;

    const StepInterval<int>		rrg_;
    const StepInterval<int>		crg_;
};


};


#endif

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "gendefs.h"
#include "paralleltask.h"
#include "executor.h"

class Coord3List;
class Coord3ListImpl;

namespace Geometry
{

class BinIDSurface;
class IndexedShape;
class ExplFaultStickSurface;
class FaultStickSet;


mExpClass(Geometry) FaultBinIDSurfaceIntersector
{
public:
				FaultBinIDSurfaceIntersector(float horshift,
					const BinIDSurface&, 
					const ExplFaultStickSurface&,
					Coord3List&);
    virtual			~FaultBinIDSurfaceIntersector();

    void			compute();	
				
				//The shape is optional, if not set, we still
				//compute intersections, stored in crdlist_    
    void			setShape(const IndexedShape&);
    const IndexedShape*		getShape();

protected:

    float			zshift_;
    Coord3List&			crdlist_;
    const BinIDSurface&		surf_;
    const IndexedShape*		output_;
    const ExplFaultStickSurface& eshape_;

private:
    void			sortPointsToLine(TypeSet<Coord3>&,
						 TypeSet<Coord3>&);
    const Coord3		findNearestPoint(const Coord3&,
						 TypeSet<Coord3>&);
    bool			findMin(TypeSet<Coord3>&,int&,bool);
    int				optimizeOrder(TypeSet<Coord3>&);

};

mExpClass(Geometry) BulkFaultBinIDSurfaceIntersector : public Executor
{ mODTextTranslationClass(BulkFaultBinIDSurfaceIntersector)
public:
				BulkFaultBinIDSurfaceIntersector(
				    float horshift,
				    BinIDSurface*,
				    ObjectSet<FaultStickSet>&,
				    ObjectSet<Coord3ListImpl>& );

				~BulkFaultBinIDSurfaceIntersector() {}

    od_int64			totalNr() const override { return totalnr_; }
    od_int64			nrDone() const override { return nrdone_; }
    uiString			uiMessage() const override
				{ return tr( "Fault-Horizon "
						"Intersector Calculator" ); }
    uiString			uiNrDoneText() const override
				    { return tr( "Faults handled" ); }

    int				nextStep() override;

    const ObjectSet<Coord3ListImpl>	getIntersectionPoints()
						{ return crdlistset_; }

protected:

    float					zshift_;
    ObjectSet<Coord3ListImpl>&			crdlistset_;
    const BinIDSurface*				surf_;
    ObjectSet<FaultStickSet>&			fssset_;
    od_int64					nriter_;

    int						totalnr_;
    int						nrdone_;
};

} // namespace Geometry

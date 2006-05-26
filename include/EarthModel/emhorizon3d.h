#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.45 2006-05-26 13:03:59 cvskris Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "binidsurface.h"

/*!
*/

class BinIDValueSet;
class BufferStringSet;

namespace EM
{

class HorizonGeometry : public RowColSurfaceGeometry
{
public:
    			HorizonGeometry( Surface& );

    const Geometry::BinIDSurface* sectionGeometry(const SectionID&) const;
    Geometry::BinIDSurface*	  sectionGeometry(const SectionID&);

    bool		removeSection(const SectionID&, bool hist);
    SectionID		cloneSection(const SectionID& sid);

    void		setShift(float);
    float		getShift() const;

    bool		isFullResolution() const;
    RowCol		loadedStep() const;
    RowCol		step() const;
    void		setStep(const RowCol& step,const RowCol& loadedstep);

    bool		enableChecks(bool yn);
    bool		isChecksEnabled() const;
    bool		isNodeOK(const PosID&) const;

    bool		isAtEdge(const PosID& pid) const;
    PosID		getNeighbor(const PosID&,const RowCol&) const;
    int			getConnectedPos(const PosID&,TypeSet<PosID>*) const;


protected:
    Geometry::BinIDSurface* createSectionGeometry() const;

    RowCol		loadedstep_;
    RowCol		step_;
    float		shift_;
    bool		checksupport_;
};


/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Horizon : public Surface
{
public:
    static const char*	typeStr();
    static EMObject*	create(EMManager&);
    static void		initClass(EMManager&);
    void		removeAll();

    HorizonGeometry&		geometry();
    const HorizonGeometry&	geometry() const;

    void		interpolateHoles(int aperture);

    const char*		getTypeStr() const { return typeStr(); }
    Executor*		importer(const ObjectSet<BinIDValueSet>&,
	    			 const RowCol& step);
    				/*!< Removes all data and creates 
				  a section for every BinIDValueSet
				*/
    Executor*		auxDataImporter(const ObjectSet<BinIDValueSet>&,
	    				const BufferStringSet& attribnms,
					const BoolTypeSet& attribsel);

    SurfaceAuxData&	auxdata;
    EdgeLineManager&	edgelinesets;

protected:
	    		Horizon(EMManager&);
	    		~Horizon();
    void		fillPar(IOPar&) const;
    bool		usePar( const IOPar& );
    const IOObjContext&	getIOObjContext() const;

    friend class	EMManager;
    friend class	EMObject;
    HorizonGeometry	geometry_;
};


}; // Namespace


#endif

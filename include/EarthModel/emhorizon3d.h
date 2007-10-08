#ifndef emhorizon3d_h
#define emhorizon3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon3d.h,v 1.54 2007-10-08 12:07:14 cvsraman Exp $
________________________________________________________________________


-*/

#include "emhorizon.h"
#include "binidsurface.h"
#include "tableascio.h"

/*!
*/

class BinIDValueSet;
class BufferStringSet;
class HorSampling;
class Scaler;
namespace Table { class FormatDesc; class AscIO; }

namespace EM
{

class Horizon3DGeometry : public HorizonGeometry
{
public:
				Horizon3DGeometry(Surface&);

    const Geometry::BinIDSurface* sectionGeometry(const SectionID&) const;
    Geometry::BinIDSurface*	sectionGeometry(const SectionID&);

    bool			removeSection(const SectionID&,bool hist);
    SectionID			cloneSection(const SectionID&);

    void			setShift(float);
    float			getShift() const;

    bool			isFullResolution() const;
    RowCol			loadedStep() const;
    RowCol			step() const;
    void			setStep(const RowCol& step,
	    				const RowCol& loadedstep);

    bool			enableChecks(bool yn);
    bool			isChecksEnabled() const;
    bool			isNodeOK(const PosID&) const;

    bool			isAtEdge(const PosID& pid) const;
    PosID			getNeighbor(const PosID&,const RowCol&) const;
    int				getConnectedPos(const PosID&,
	    					TypeSet<PosID>*) const;

    void			fillBinIDValueSet(const SectionID&,
	    					  BinIDValueSet&) const;

    EMObjectIterator*   	createIterator(const EM::SectionID&,
					       const CubeSampling* =0) const;

protected:
    Geometry::BinIDSurface*	createSectionGeometry() const;

    RowCol			loadedstep_;
    RowCol			step_;
    float			shift_;
    bool			checksupport_;
};


/*!\brief
The horizon is made up of one or more grids (so they can overlap at faults).
The grids are defined by knot-points in a matrix and the fillstyle inbetween
the knots.
*/

class Horizon3D : public Horizon
{ mDefineEMObjFuncs( Horizon3D );
public:

    void			removeAll();
    Horizon3DGeometry&		geometry();
    const Horizon3DGeometry&	geometry() const;

    Array2D<float>*		createArray2D(SectionID);
    bool			setArray2D(const Array2D<float>&,SectionID,
	    				   bool onlyfillundefs);
    				//!< Returns true on succes

    Executor*			importer(const ObjectSet<BinIDValueSet>&,
	    				 const HorSampling& hs);
    					/*!< Removes all data and creates 
					  a section for every BinIDValueSet
					*/
    Executor*			auxDataImporter(const ObjectSet<BinIDValueSet>&,
	    				const BufferStringSet& attribnms,int,
					const HorSampling& hs);

    SurfaceAuxData&		auxdata;
    EdgeLineManager&		edgelinesets;

protected:
    void			fillPar(IOPar&) const;
    bool			usePar( const IOPar& );
    const IOObjContext&		getIOObjContext() const;

    friend class		EMManager;
    friend class		EMObject;
    Horizon3DGeometry		geometry_;
};


class Horizon3DAscIO : public Table::AscIO
{
public:
    				Horizon3DAscIO( const Table::FormatDesc& fd,
						std::istream& strm )
				    : Table::AscIO(fd)
				    , strm_(strm)      		    {}

    static Table::FormatDesc*   getDesc(bool);
    static void			updateDesc(Table::FormatDesc&,
	    				   const BufferStringSet&,bool);
    static void                 createDescBody(Table::FormatDesc*,
	    				   const BufferStringSet&,bool);

    float			getUdfVal();
    int				getNextLine(TypeSet<float>&);
protected:

    std::istream&		strm_;
};


}; 

// namespace EarthModel

#endif

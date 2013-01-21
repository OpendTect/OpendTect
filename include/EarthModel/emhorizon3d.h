#ifndef emhorizon3d_h
#define emhorizon3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emhorizon.h"
#include "binidsurface.h"
#include "tableascio.h"

class BinIDValueSet;
class DataPointSet;
class BufferStringSet;
class HorSampling;
class Scaler;
class ZAxisTransform;
namespace Table { class FormatDesc; }
namespace Pick { class Set; }
namespace Pos { class Provider3D; }

namespace EM
{

/*!
\brief 3D HorizonGeometry
*/

mExpClass(EarthModel) Horizon3DGeometry : public HorizonGeometry
{
public:
				Horizon3DGeometry(Surface&);

    const Geometry::BinIDSurface* sectionGeometry(const SectionID&) const;
    Geometry::BinIDSurface*	sectionGeometry(const SectionID&);

    bool			removeSection(const SectionID&,bool hist);
    SectionID			cloneSection(const SectionID&);

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

    bool			getBoundingPolygon(const SectionID&,
	    					   Pick::Set&) const;
    void			getDataPointSet( const SectionID&,
	    					  DataPointSet&,
						  float shift=0.0) const;
    void			fillBinIDValueSet(const SectionID&,
	    					 BinIDValueSet&,
						 Pos::Provider3D* prov=0) const;

    EMObjectIterator*   	createIterator(const EM::SectionID&,
					       const CubeSampling* =0) const;
protected:

    Geometry::BinIDSurface*	createSectionGeometry() const;

    RowCol			loadedstep_;
    RowCol			step_;
    bool			checksupport_;
};


/*!
\brief 3D Horizon. A Horizon is made up of one or more grids (so they can
overlap at faults). The grids are defined by knot-points in a matrix and
fillstyle in between the knots.
*/

mExpClass(EarthModel) Horizon3D : public Horizon
{ mDefineEMObjFuncs( Horizon3D );
public:

    float			getZ(const BinID&) const;
				//!< Fast: reads from the first section
    bool			setZ(const BinID&,float z,bool addtohist);
				//!< Fast: writes to the first section
    virtual float		getZValue(const Coord&,bool allow_udf=true,
	    				  int nr=0) const;
				//!< Slow: if you need the choices
    HorSampling			range(SectionID sid=-1) const;

    void			removeAll();
    Horizon3DGeometry&		geometry();
    const Horizon3DGeometry&	geometry() const;

    Array2D<float>*		createArray2D(SectionID,
					      const ZAxisTransform* zt=0) const;
    bool			setArray2D(const Array2D<float>&,SectionID,
	    				   bool onlyfillundefs,
					   const char* histdesc);
    				/*!< Returns true on succes.  If histdesc
				     is set, the event will be saved to
				     history with the desc. */

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


/*!
\brief Ascii I/O for Horizon3D.
*/

mExpClass(EarthModel) Horizon3DAscIO : public Table::AscIO
{
public:
    				Horizon3DAscIO( const Table::FormatDesc& fd,
						std::istream& strm )
				    : Table::AscIO(fd)
				    , udfval_(mUdf(float))
				    , finishedreadingheader_(false)
				    , strm_(strm)      		    {}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&,
	    				   const BufferStringSet&);
    static void                 createDescBody(Table::FormatDesc*,
	    				   const BufferStringSet&);

    bool			isXY() const;
    int				getNextLine(Coord&,TypeSet<float>&);

    static const char*		sKeyFormatStr();
    static const char*		sKeyAttribFormatStr();

protected:

    std::istream&		strm_;
    float			udfval_;
    bool			finishedreadingheader_;
};


} // namespace EM

#endif


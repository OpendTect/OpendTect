#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emhorizon.h"
#include "horizon2dline.h"

class ZAxisTransform;
namespace Geometry { class Horizon2DLine; }
namespace Table { class FormatDesc; }
template <class T> class Array1D;

namespace EM
{
class EMManager;

/*!
\brief 2D HorizonGeometry
*/

mExpClass(EarthModel) Horizon2DGeometry : public HorizonGeometry
{
public:
				Horizon2DGeometry(Surface&);
				~Horizon2DGeometry();

    Geometry::Horizon2DLine*	geometryElement() override;
    const Geometry::Horizon2DLine* geometryElement() const override;

    int				nrLines() const;

    int				lineIndex(Pos::GeomID geomid) const;

    int				lineIndex(const char* linenm) const;
    const char*			lineName(int id) const;
    const char*			lineSet(int id) const;
    Pos::GeomID			geomID(int idx) const;
    void			getGeomIDs(TypeSet<Pos::GeomID>&) const;
    bool			hasLine(Pos::GeomID) const;

    PosID			getPosID(const TrcKey&) const override;
    TrcKey			getTrcKey(const PosID&) const override;

    bool			includeLine(Pos::GeomID geomid,int step=1);

    bool			addLine(Pos::GeomID geomid,int step=1);

    bool			addLine(Pos::GeomID geomid,
					const StepInterval<int>& trcrg);

    void			removeLine(Pos::GeomID geomid);

    bool			isAtEdge(const PosID&) const override;
    PosID			getNeighbor(const PosID& posid,bool nextcol,
					    bool retundef=false) const;
				/*!<
				  \param posid
				  \param nextcol
				  \param retundef specifies what to do
					   if no neighbor is found. If it
					   true, it returnes unf, if not
					   it return the id of the undef
					   neighbor. */

    static const char*		sKeyLineIDs()	{ return "Line IDs"; }
    static const char*		sKeyLineNames() { return "Line names"; }
    static const char*		sKeyLineSets()	{ return "Set ID of line "; }
    static const char*		sKeyID()	{ return "ID"; }
    static const char*		sKeyTraceRange()
				{ return "Trace Range of line "; }
    static const char*		sKeyTrcRg()	{ return "Trace range"; }
    static const char*		sKeyNrLines()	{ return "Nr of Lines"; }

    int				getConnectedPos(const PosID& posid,
					    TypeSet<PosID>* res) const override;
    StepInterval<int>		colRange(Pos::GeomID geomid) const;

// Deprecated public functions
    mDeprecated("Use geometryElement()")
    Geometry::Horizon2DLine*	sectionGeometry(const SectionID&) override
				{ return geometryElement(); }
    mDeprecated("Use geometryElement() const")
    const Geometry::Horizon2DLine* sectionGeometry(
					const SectionID&) const override
				{ return geometryElement(); }
    mDeprecated("Use colRange() without SectionID")
    StepInterval<int>		colRange(const SectionID&,
					 Pos::GeomID geomid) const
				{ return colRange(geomid); }


protected:
    Geometry::Horizon2DLine*	createGeometryElement() const override;

    bool			doAddLine(Pos::GeomID geomid,
					  const StepInterval<int>& trcrg,
					  bool mergewithdouble);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

};


/*!
\brief 2D Horizon. The Horizon is only present along 2d lines, set by addLine.
Each position's subid is formed by RowCol( lineid, tracenr ).getInt64(). If
multiple z-values per trace is needed, multiple sections can be added.
*/

mExpClass(EarthModel) Horizon2D : public Horizon
{ mDefineEMObjFuncs( Horizon2D );
public:

    float			getZ(const TrcKey&) const override;
    bool			setZ(const TrcKey&,float z,
				     bool addtohist) override;
    bool			setZAndNodeSourceType(const TrcKey&,
					  float z,bool addtohist,
					  NodeSourceType type=Auto) override;
    bool			hasZ(const TrcKey&) const override;
    Coord3			getCoord(const TrcKey&) const override;
    void			setAttrib(const TrcKey&,int attr,int yn,
					  bool addtohist) override;
    bool			isAttrib(const TrcKey&,int attr) const override;

    float			getZValue(const Coord&,bool allow_udf=true,
					  int nr=0) const override;
				//!< Convenience function. If you need speed,
				//!< don't use it.

    bool			unSetPos(const EM::PosID&,
					 bool addtohist) override;
    bool			unSetPos(const EM::SubID&,
					 bool addtohist) override;
    Coord3			getPos(const EM::PosID&) const override;
    Coord3			getPos(const EM::SubID&) const override;
    TypeSet<Coord3>		getPositions(int lineidx,int trcnr) const;
    Coord3			getPosition(int lidx,int trcnr) const;

    Coord3			getPos(Pos::GeomID,int trc) const;
    void			setNodeSourceType(const TrcKey&,
					NodeSourceType) override;
    bool			isNodeSourceType(const PosID&,
					NodeSourceType)const override;
    bool			isNodeSourceType(const TrcKey&,
					NodeSourceType)const override;

    bool			setPos(Pos::GeomID geomid,
				       int trcnr,float z,bool addtohist);

    bool			setPos(const EM::PosID&,const Coord3&,
					bool addtohist) override;
    bool			setPos(const EM::SubID&,
				       const Coord3&,bool addtohist) override;

    Horizon2DGeometry&		geometry() override	{ return geometry_; }
    const Horizon2DGeometry&	geometry() const override { return geometry_; }

    void			removeAll() override;
    void			removeSelected(const Selector<Coord3>& selector,
					       TaskRunner* tr ) override;

    bool			setArray1D(const Array1D<float>&,
					   const StepInterval<int>& trcrg,
					   Pos::GeomID geomid,
					   bool onlyfillundefs);
    bool			setArray1D(const Array1D<float>&,
					   Pos::GeomID geomid,
					   bool onlyfillundefs );
    Array1D<float>*		createArray1D(Pos::GeomID geomid,
					      const ZAxisTransform* =0) const;

    OD::GeomSystem		getSurveyID() const override;
    uiString			getUserTypeStr() const override
				    { return userTypeStr(); }
    static uiString		userTypeStr() { return tr("2D Horizon"); }

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =nullptr) const override;

// Deprecated public functions
    mDeprecated("Use without SectionID")
    Coord3			getPosition(EM::SectionID,int lidx,
					    int trcnr) const
				{ return getPosition(lidx,trcnr); }
    mDeprecated("Use without SectionID")
    Coord3			getPos(EM::SectionID,Pos::GeomID geomid,
					int trc) const
				{ return getPos(geomid,trc); }
    mDeprecated("Use without SectionID")
    bool			setArray1D(const Array1D<float>& arr,
					   const StepInterval<int>& trcrg,
					   SectionID sid,Pos::GeomID geomid,
					   bool onlyfillundefs)
				{ return setArray1D(arr,trcrg,geomid,
						    onlyfillundefs); }
    mDeprecated("Use without SectionID")
    bool			setArray1D(const Array1D<float>& arr,
					   SectionID sid,Pos::GeomID geomid,
					   bool onlyfillundefs)
				{ return setArray1D(arr,geomid,onlyfillundefs);}
    mDeprecated("Use without SectionID")
    Array1D<float>*		createArray1D(SectionID,
					      Pos::GeomID geomid,
					      const ZAxisTransform* zt=0) const
				{ return createArray1D(geomid,zt); }

protected:

    const IOObjContext&		getIOObjContext() const override;
    void			initNodeSourceArray(const TrcKey&);

    Horizon2DGeometry		geometry_;
    Array1D<char>*		nodesource_;
				/*!< '0'- non interpreted,
				'1'- manual interpreted,
				'2' - auto interpreted.
				'3' - Gridding.
				enum NodeSourceType*/
    StepInterval<int>	    trackingsampling_;
};

} // namespace EM

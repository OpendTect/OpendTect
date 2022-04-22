#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emhorizon.h"
#include "bufstringset.h"
#include "horizon2dline.h"
#include "posinfo2dsurv.h"

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
    Geometry::Horizon2DLine*	sectionGeometry(const SectionID&) override;
    const Geometry::Horizon2DLine* sectionGeometry(
					    const SectionID&) const override;

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
    StepInterval<int>		colRange(const SectionID&,
					 Pos::GeomID geomid) const;

    StepInterval<int>		colRange(Pos::GeomID geomid) const;

protected:
    Geometry::Horizon2DLine*	createSectionGeometry() const override;

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
    bool			unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohist) override;
    Coord3			getPos(const EM::PosID&) const override;
    Coord3			getPos(const EM::SectionID&,
				       const EM::SubID&) const override;
    TypeSet<Coord3>		getPositions(int lineidx,int trcnr) const;
    Coord3			getPosition(EM::SectionID,int lidx,
					    int trcnr) const;

    Coord3			getPos(EM::SectionID,Pos::GeomID,int trc) const;
    void			setNodeSourceType(const TrcKey&,
					NodeSourceType) override;
    bool			isNodeSourceType(const PosID&,
					NodeSourceType)const override;
    bool			isNodeSourceType(const TrcKey&,
					NodeSourceType)const override;

    bool			setPos(EM::SectionID,Pos::GeomID geomid,
				       int trcnr,float z,bool addtohist);

    bool			setPos(const EM::PosID&,const Coord3&,
					bool addtohist) override;
    bool			setPos(const EM::SectionID&,const EM::SubID&,
				       const Coord3&,bool addtohist) override;

    Horizon2DGeometry&		geometry() override	{ return geometry_; }
    const Horizon2DGeometry&	geometry() const override { return geometry_; }

    void			removeAll() override;
    void			removeSelected(const Selector<Coord3>& selector,
					       TaskRunner* tr ) override;

    bool			setArray1D(const Array1D<float>&,
					   const StepInterval<int>& trcrg,
					   SectionID sid,Pos::GeomID geomid,
					   bool onlyfillundefs);
    bool			setArray1D(const Array1D<float>&,SectionID sid,
					   Pos::GeomID geomid,
					   bool onlyfillundefs );

    Array1D<float>*		createArray1D(SectionID,
					      Pos::GeomID geomid,
					      const ZAxisTransform* =0) const;

    OD::GeomSystem		getSurveyID() const override;
    uiString			getUserTypeStr() const override
				    { return userTypeStr(); }
    static uiString		userTypeStr() { return tr("2D Horizon"); }

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


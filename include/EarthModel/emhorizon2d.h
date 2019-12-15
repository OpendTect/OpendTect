#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emhorizon.h"
#include "horizon2dline.h"
#include "posinfo2dsurv.h"

class ZAxisTransform;
namespace Geometry { class Horizon2DLine; }
namespace Table { class FormatDesc; }
template <class T> class Array1D;

namespace EM
{

/*!\brief 2D HorizonGeometry */

mExpClass(EarthModel) Horizon2DGeometry : public HorizonGeometry
{
public:
				Horizon2DGeometry(Surface&);
    Geometry::Horizon2DLine*	geometryElement();
    const Geometry::Horizon2DLine* geometryElement() const;

    int				nrLines() const;

    int				lineIndex(Pos::GeomID geomid) const;

    int				lineIndex(const char* linenm) const;
    const char*			lineName(int id) const;
    const char*			lineSet(int id) const;
    Pos::GeomID			geomID(int idx) const;
    void			setGeomID(int idx,Pos::GeomID);

    PosID			getPosID(const TrcKey&) const;
    TrcKey			getTrcKey(const PosID&) const;

    bool			includeLine(Pos::GeomID geomid,int step=1);

    bool			addLine(Pos::GeomID geomid,int step=1);

    bool			addLine(Pos::GeomID geomid,
					const StepInterval<int>& trcrg);

    void			removeLine(Pos::GeomID geomid);

    bool			isAtEdge(const PosID&) const;
    PosID			getNeighbor(const PosID&,bool nextcol,
					    bool retundef=false) const;
				/*!<\param retundef specifies what to do
				           if no neighbor is found. If it
					   true, it returnes unf, if not
					   it return the id of the undef
					   neighbor. */

    static const char*		sKeyLineIDs()	{ return "Line IDs"; }
    static const char*		sKeyLineNames()	{ return "Line names"; }
    static const char*		sKeyLineSets()	{ return "Set ID of line "; }
    static const char*		sKeyID()	{ return "ID"; }
    static const char*		sKeyTraceRange()
				{ return "Trace Range of line "; }
    static const char*		sKeyTrcRg()	{ return "Trace range"; }
    static const char*		sKeyNrLines()	{ return "Nr of Lines"; }

    int				getConnectedPos(const PosID& posid,
						TypeSet<PosID>* res) const;
    StepInterval<int>		colRange(Pos::GeomID geomid) const;

protected:
    Geometry::Horizon2DLine*	createGeometryElement() const;

    bool			doAddLine(Pos::GeomID geomid,
					  const StepInterval<int>& trcrg,
					  bool mergewithdouble);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    GeomIDSet	geomids_;

};


/*!
\brief 2D Horizon. The Horizon is only present along 2d lines, set by addLine.
Each position's subid is formed by RowCol( lineid, tracenr ).getInt64(). If
multiple z-values per trace is needed, multiple sections can be added.
*/

mExpClass(EarthModel) Horizon2D : public Horizon
{   mDefineEMObjFuncs( Horizon2D );
    mODTextTranslationClass( Horizon2D );
public:

    virtual bool		is2D() const		{ return true; }

    virtual float		getZ(const TrcKey&) const;
    virtual bool		setZ(const TrcKey&,float z,bool addtohist,
				     NodeSourceType type=Auto);
    virtual bool		hasZ(const TrcKey&) const;
    virtual Coord3		getCoord(const TrcKey&) const;
    virtual void		setAttrib(const TrcKey&,int attr,bool yn,
					  bool addtohist);
    virtual bool		isAttrib(const TrcKey&,int attr) const;

    virtual float		getZValue(const Coord&,bool allow_udf=true,
					  int nr=0) const;
				//!< Convenience function. If you need speed,
				//!< don't use it.

    bool			unSetPos(const EM::PosID&,bool addtohist);
    Coord3			getPos(const EM::PosID&) const;
    Coord3			getPosition(int lineidx,int trcnr) const;

    Coord3			getPos(Pos::GeomID,int trc) const;

    virtual void		setNodeSourceType(const TrcKey&,
					NodeSourceType);
    virtual bool		isNodeSourceType(const PosID&,
					NodeSourceType)const;
    virtual bool		isNodeSourceType(const TrcKey&,
					NodeSourceType)const;

    bool			setZPos(Pos::GeomID geomid,
				       int trcnr,float z,bool addtohist,
				       NodeSourceType type=Auto);

    Horizon2DGeometry&		geometry()		{ return geometry_; }
    const Horizon2DGeometry&	geometry() const	{ return geometry_; }

    virtual void		removeAll();
    void			removeSelected(const Selector<Coord3>&,
					       const TaskRunnerProvider&);

    bool			setArray1D(const Array1D<float>&,
					   const StepInterval<int>& trcrg,
					   Pos::GeomID geomid,
					   bool onlyfillundefs);
    bool			setArray1D(const Array1D<float>&,
					   Pos::GeomID geomid,
					   bool onlyfillundefs );

    Array1D<float>*		createArray1D(Pos::GeomID geomid,
					      const ZAxisTransform* =0) const;

    OD::GeomSystem		geomSystem() const
				{ return OD::LineBasedGeom; }
    uiString			getUserTypeStr() const { return userTypeStr(); }
    static uiString		userTypeStr() { return tr("2D Horizon"); }

protected:

    const IOObjContext&		getIOObjContext() const;
    void			initNodeSourceArray(const TrcKey&);

    Horizon2DGeometry		geometry_;
    Array1D<char>*		nodesource_;
				/*!< '0'- non interpreted,
				'1'- manual interpreted,
				'2' - auto interpreted. see
				 enum NodeSourceType*/
};

} // namespace EM

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emfault.h"
#include "tableascio.h"
#include "emfaultstickset.h"

namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }
namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }

namespace EM
{
class EMManager;
class FaultAuxData;

/*!
\brief 3D FaultGeometry.
*/

mExpClass(EarthModel) Fault3DGeometry : public FaultGeometry
{
public:
			Fault3DGeometry(Surface&);
			~Fault3DGeometry();

    int			nrSticks() const;
    int			nrKnots(int sticknr) const;

    bool		insertStick(int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override;
    bool		removeStick(int sticknr,
				    bool addtohistory) override;
    bool		insertKnot(const SubID&,
			       const Coord3& pos,bool addtohistory) override;
    bool		removeKnot(const SubID&,
				   bool addtohistory) override;

    bool		areSticksVertical() const;
    bool		areEditPlanesMostlyCrossline() const;

    Geometry::FaultStickSurface*	geometryElement() override;
    const Geometry::FaultStickSurface*	geometryElement() const override;

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =nullptr) const override;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

// Deprecated public functions
    mDeprecated("Use geometryElement()")
    Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) override
			{ return geometryElement(); }
    mDeprecated("Use geometryElement() const")
    const Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) const override
			{ return geometryElement(); }

    mDeprecated("Use without SectionID")
    int			nrSticks(const SectionID&) const
			{ return nrSticks(); }
    mDeprecated("Use without SectionID")
    int			nrKnots(const SectionID&,int sticknr) const
			{ return nrKnots(sticknr); }

    mDeprecated("Use without SectionID")
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override
			{ return insertStick(sticknr,firstcol,pos,
					     editnormal,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removeStick(const SectionID&,int sticknr,
				    bool addtohistory) override
			{ return removeStick(sticknr,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		insertKnot(const SectionID&,const SubID& subid,
			       const Coord3& pos,bool addtohistory) override
			{ return insertKnot(subid,pos,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		removeKnot(const SectionID&,const SubID& subid,
				   bool addtohistory) override
			{ return removeKnot(subid,addtohistory); }
    mDeprecated("Use without SectionID")
    bool		areSticksVertical(const SectionID&) const
			{ return areSticksVertical(); }


protected:
    Geometry::FaultStickSurface*	createGeometryElement() const override;
};


/*!
\brief 3D Fault
*/

mExpClass(EarthModel) Fault3D : public Fault
{ mDefineEMObjFuncs( Fault3D );
public:
    Fault3DGeometry&		geometry() override;
    const Fault3DGeometry&	geometry() const override;
    void			apply(const Pos::Filter&) override;
    uiString			getUserTypeStr() const override;

    FaultAuxData*		auxData();
    const FaultAuxData*		auxData() const;

    EMObjectIterator*	createIterator(
				const TrcKeyZSampling* =nullptr) const override;

protected:

    const IOObjContext&		getIOObjContext() const override;

    friend class		EMManager;
    friend class		EMObject;
    Fault3DGeometry		geometry_;
    FaultAuxData*		auxdata_;
};


/*!
\brief Ascii I/O for Fault.
*/

mExpClass(EarthModel) FaultAscIO : public Table::AscIO
{
public:
				FaultAscIO(const Table::FormatDesc&);
				~FaultAscIO();

    static Table::FormatDesc*	getDesc(bool is2d);

    bool			get(od_istream&,EM::Fault&,
				    bool sortsticks=false,
				    bool is2d=false) const;
protected:
    bool			isXY() const;
};


/*!
\brief Class to hold Fault-stick coordinates and compute the normal.
*/

mExpClass(EarthModel) FaultStick
{
public:
			FaultStick(int stickidx);
			~FaultStick();

    Coord3		getNormal(bool is2d) const;

    int			stickidx_;
    TypeSet<Coord3>	crds_;
    BufferString	lnm_;
};

} // namespace EM

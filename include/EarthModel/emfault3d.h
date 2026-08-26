#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "emfault.h"
#include "locationbase.h"
#include "tableascio.h"

namespace Table { class FormatDesc; }

namespace Geometry { class FaultStickSurface; }
namespace Geometry { class FaultStickSet; }
namespace Pos { class Filter; }
namespace ZDomain { class Def; }

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

    Fault3DGeometry&	operator =(const Fault3DGeometry&);

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

    TrcKeyZSampling	getEnvelope() const override { return tkzsenvelope_; }

// Deprecated public functions
mStartAllowDeprecatedSection
    mDeprecated("Use geometryElement()")
    Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) override;
    mDeprecated("Use geometryElement() const")
    const Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) const override;
mStopAllowDeprecatedSection

    mDeprecated("Use without SectionID")
    int			nrSticks(const SectionID&) const
			{ return nrSticks(); }
    mDeprecated("Use without SectionID")
    int			nrKnots(const SectionID&,int sticknr) const
			{ return nrKnots(sticknr); }

mStartAllowDeprecatedSection
    mDeprecated("Use without SectionID")
    bool		insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory) override;
    mDeprecated("Use without SectionID")
    bool		removeStick(const SectionID&,int sticknr,
				    bool addtohistory) override;
    mDeprecated("Use without SectionID")
    bool		insertKnot(const SectionID&,const SubID& subid,
			       const Coord3& pos,bool addtohistory) override;
    mDeprecated("Use without SectionID")
    bool		removeKnot(const SectionID&,const SubID& subid,
				   bool addtohistory) override;
mStopAllowDeprecatedSection
    mDeprecated("Use without SectionID")
    bool		areSticksVertical(const SectionID&) const;


protected:
    Geometry::FaultStickSurface*	createGeometryElement() const override;
    void			setEnvelope(const TrcKeyZSampling&) override;

    TrcKeyZSampling			tkzsenvelope_;

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

    EMObjectIterator*		createIterator(
				const TrcKeyZSampling* =nullptr) const override;

protected:

    const IOObjContext&		getIOObjContext() const override;

    friend class		EMManager;
    friend class		EMObject;
    Fault3DGeometry		geometry_;
    FaultAuxData*		auxdata_    = nullptr;
};


/*!
\brief Ascii I/O for Fault.
*/

mExpClass(EarthModel) FaultAscIO : public Table::AscIO
{
public:
				FaultAscIO(const Table::FormatDesc&);
				~FaultAscIO();

    static Table::FormatDesc*	getDesc(bool is2dq,const ZDomain::Def&);
    static void			updateDesc(Table::FormatDesc&,bool is2d,
						const ZDomain::Def&);
    static void			createDescBody(Table::FormatDesc*,bool is2d,
						    const ZDomain::Def&);

    bool			get(od_istream&,EM::Fault&,
				    bool sortsticks=false,
				    bool is2d=false) const;
protected:

    bool			isXY() const;
};


mExpClass(EarthModel) FaultDataUpdater : public ParallelTask
{ mODTextTranslationClass(FaultDataUpdater)
public:
			    FaultDataUpdater(Geometry::FaultStickSurface&);
			    ~FaultDataUpdater();

    TrcKeyZSampling		getEnvelope() const { return tkzsenvelope_; }

protected:
    bool			    doWork(od_int64,od_int64,int) override;
    bool			    doFinish(bool) override;

    od_int64			    nrIterations() const override;
    Geometry::FaultStickSurface&    faultsurf_;
    od_int64			    totnr_		= 0;
    TypeSet<TrcKeyZSampling>	    tkzsset_;
    TrcKeyZSampling		    tkzsenvelope_;
};




} // namespace EM

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emmanager.h"
#include "emposid.h"
#include "stratlevel.h"

class BufferStringSet;
class TrcKeyZSampling;
class IOObj;
class UnitOfMeasure;
namespace ZDomain { class Info; }

namespace EM
{

class dgbSurfaceReader;
class SurfaceIOData;

/*!
\brief Info on IOObj for earthmodel.
*/

mExpClass(EarthModel) IOObjInfo
{ mODTextTranslationClass(IOObjInfo);
public:

			IOObjInfo(const IOObj*);
			IOObjInfo(const IOObj&);
			IOObjInfo(const MultiID&);
			IOObjInfo(const IOObjInfo&);
    virtual		~IOObjInfo();

    IOObjInfo&		operator =(const IOObjInfo&);

    static void		getIDs(ObjectType,TypeSet<MultiID>&);
			//!< Does not erase the IDs at start

    bool		isOK() const;
    inline const IOObj*	ioObj() const		{ return ioobj_; }
    const char*		name() const;
    inline ObjectType	type() const		{ return type_; }

    bool		getSectionIDs(TypeSet<SectionID>&) const;
    bool		getSectionNames(BufferStringSet&) const;
    bool		getAttribNames(BufferStringSet&) const;
    Interval<float>	getZRange() const;
    BufferString	getZUnitLabel() const;
    const UnitOfMeasure* getZUoM() const;
    StepInterval<int>	getInlRange() const;
    StepInterval<int>	getCrlRange() const;
    IOPar*		getPars() const;
    int			getParsOffsetInFile() const;
    uiString		getMessage() const;
    const char*		timeLastModified() const;
    const char*		timeLastModified(bool iso) const;
    const ZDomain::Info& zDomain() const;

    // Surface
    inline bool		isSurface() const
					{ return type_ != ObjectType::Body; }
    bool		getSurfaceData(SurfaceIOData&,uiString& err) const;

    // Horizon
    Strat::LevelID	levelID() const;
    static void		getTiedToLevelID(Strat::LevelID,TypeSet<MultiID>&,
					 bool is2d);
    static bool		sortHorizonsOnZValues(const TypeSet<MultiID>&,
					      TypeSet<MultiID>&);

    // 2D Horizons
    bool		getLineNames(BufferStringSet&) const;
    bool		getGeomIDs(TypeSet<Pos::GeomID>&) const;
    bool		getTrcRanges(TypeSet< StepInterval<int> >&) const;
    bool		hasGeomIDs() const;

    static void		getHorizonNamesForLine(const Pos::GeomID&,
					       BufferStringSet&);
    static void		getHorizonIDsForLine(const Pos::GeomID&,
					     TypeSet<MultiID>&);

    // Body
    bool		getBodyRange(TrcKeyZSampling&) const;

    // FaultStickSet
    int			nrSticks() const;

    // Helpful stuff
    static ObjectType	objectTypeOfIOObjGroup(const char*);

protected:

    ObjectType			type_;
    IOObj*			ioobj_;
    const ZDomain::Info*	zinfo_	= nullptr;
    mutable dgbSurfaceReader*	reader_ = nullptr;

    void			fillZDomain();
    void			init();

    void			setType();
};

} // namespace EM

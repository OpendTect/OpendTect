#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "crsproj.h"
#include "coordsystem.h"


namespace Coords
{

mExpClass(CRS) ProjectionBasedSystem : public CoordSystem
{ mODTextTranslationClass(ProjectionBasedSystem);
public:

    mDefaultFactoryInstantiation( CoordSystem,ProjectionBasedSystem,
				  sKey::ProjSystem(),
				 tr("Projection Based System") );

				ProjectionBasedSystem();
				ProjectionBasedSystem(const AuthorityCode&);
				~ProjectionBasedSystem();

    CoordSystem*	clone() const override;
    CoordSystem*	getGeodeticSystem() const override;
    static CoordSystem* getWGS84LLSystem();

    uiString		description() const override
			{ return tr("Projection based Coordinate System"); }
    BufferString	summary() const override;

    bool		isOK() const override;

    bool		geographicTransformOK() const override;

    BufferString	toString(StringType=Default,
				 bool withsystem=false) const override;
			//!< Default for proj: URN
    RefMan<CoordSystem> fromString(const char*,
				   BufferString* msg=nullptr) const override;

    bool		isOrthogonal() const override;
    bool		isLatLong() const override;
    bool		isProjection() const override		{ return true; }
    bool		isFeet() const override;
    bool		isMeter() const override;
    bool		isWGS84() const override;
    BufferString	getUnitName() const override;

    BufferString	getURNString() const;
    BufferString	getWKTString() const;
    BufferString	getJSONString() const;
    BufferString	getUrl() const;

    bool		setProjection(const AuthorityCode&);
    const Projection*	getProjection() const;

    Coord		convertFrom(const Coord&,
				    const CoordSystem& from) const override;
    Coord		convertTo(const Coord&,
				  const CoordSystem& to) const override;

protected:

    Projection*		proj_	= nullptr;

private:

    LatLong		toGeographic(const Coord&,bool wgs84) const override;
    Coord		fromGeographic(const LatLong&,
				       bool wgs84) const override;
    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;

};

} // namespace Coords

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
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
				  "ProjectionBased System",
				 tr("Projection Based System") );

				ProjectionBasedSystem();
				ProjectionBasedSystem(AuthorityCode);

    CoordSystem*	clone() const override;

    uiString		description() const override
				{ return tr("Geographical Coordinate System"); }
    BufferString	summary() const override;

    bool		isOK() const override;

    bool		geographicTransformOK() const override;

    bool		isOrthogonal() const override;
    bool		isProjection() const override		{ return true; }
    bool		isFeet() const override;
    bool		isMeter() const override;
    BufferString	getURNString() const override;

    bool			setProjection(AuthorityCode);
    const Projection*		getProjection() const;

protected:

    const Projection*		proj_	= nullptr;

private:

    LatLong		toGeographic(const Coord&,bool wgs84) const override;
    Coord		fromGeographic(const LatLong&,
						    bool wgs84) const override;
    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;

};

}; //namespace

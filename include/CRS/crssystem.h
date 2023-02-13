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
				  sKey::ProjSystem(),
				 tr("Projection Based System") );

				ProjectionBasedSystem();
				ProjectionBasedSystem(AuthorityCode);

    virtual CoordSystem*	clone() const;

    virtual uiString		description() const
				{ return tr("Geographical Coordinate System"); }
    virtual BufferString	summary() const;

    virtual bool		isOK() const;

    virtual bool		geographicTransformOK() const;

    virtual bool		isOrthogonal() const;
    virtual bool		isProjection() const		{ return true; }
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    bool			setProjection(AuthorityCode);
    const Projection*		getProjection() const;

protected:

    const Projection*		proj_;

private:

    virtual LatLong		toGeographic(const Coord&,
					     bool wgs84) const;
    virtual Coord		fromGeographic(const LatLong&,
					       bool wgs84) const;
    virtual bool		doUsePar(const IOPar&);
    virtual void		doFillPar(IOPar&) const;

};

}; //namespace

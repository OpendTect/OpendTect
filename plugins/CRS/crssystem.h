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

mExpClass(CRS) ProjectionBasedSystem : public PositionSystem
{ mODTextTranslationClass(ProjectionBasedSystem);
public:

    mDefaultFactoryInstantiation( PositionSystem,ProjectionBasedSystem,
				  "ProjectionBased System",
				 tr("Projection Based System") );

				ProjectionBasedSystem();
				ProjectionBasedSystem(ProjectionID);

    virtual PositionSystem*	clone() const;

    virtual uiString		description() const
				{ return tr("Geographical Coordinate System"); }
    virtual BufferString	summary() const;

    virtual bool		isOK() const;

    virtual bool		geographicTransformOK() const;

    virtual bool		isOrthogonal() const;
    virtual bool		isProjection() const		{ return true; }
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    bool			setProjection(ProjectionID);
    const Projection*		getProjection() const;

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

protected:

    const Projection*		proj_;

private:

    virtual LatLong		toGeographic(const Coord&,
					     bool wgs84=false) const;
    virtual Coord		fromGeographic(const LatLong&,
					       bool wgs84=false) const;

};

}; //namespace

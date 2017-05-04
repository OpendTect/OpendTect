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


    virtual uiString		description() const
				{ return tr("Geographical Coordinate System"); }

    virtual bool		isOK() const;

    virtual bool		geographicTransformOK() const;
    virtual LatLong		toGeographicWGS84(const Coord&) const;
    virtual Coord		fromGeographicWGS84(const LatLong&) const;

    virtual bool		isOrthogonal() const;
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    bool			setProjection(ProjectionID);
    const Projection*		getProjection() const;

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

protected:

    const Projection*		proj_;
};

}; //namespace

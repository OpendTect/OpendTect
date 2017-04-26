#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "crsmod.h"
#include "coordsystem.h"


namespace Coords
{

mExpClass(CRS) ProjectionBasedSystem : public PositionSystem
{ mODTextTranslationClass(ProjectionBasedSystem);
public:

    				ProjectionBasedSystem();


    virtual uiString		description() const
				{ return tr("Geographical Coordinate System"); }

    virtual bool		isOK() const;

    virtual bool		geographicTransformOK() const;
    virtual LatLong		toGeographicWGS84(const Coord&) const;
    virtual Coord		fromGeographicWGS84(const LatLong&) const;

    virtual uiString		toUiString(const Coord&) const;
    virtual BufferString	toString(const Coord&,
					 bool withsystem=false) const;
				/*!<Returns string. If withsystem is turned on
				    it will start with the factory name of the
				    system, followed by a space. */
    virtual Coord		fromString(const char*) const;

    virtual bool		isOrthogonal() const;
    virtual bool		isFeet() const;
    virtual bool		isMeter() const;

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

};

}; //namespace

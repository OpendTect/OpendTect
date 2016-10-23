#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2016
________________________________________________________________________

-*/

#include "factory.h"

#include "bufstring.h"
#include "coord.h"
#include "latlong.h"
#include "refcount.h"

class NotifierAccess;

namespace Coords
{



/*! Base class for Coordinate Systems, these are all two-dimensional and
    coordinates can be stored in Coord. They may use any projection, but they
    must be able to return Geographic coordinates using the WGS84 datum.  */

mExpClass(Basic) PositionSystem : public RefCount::Referenced
{
public:

    bool			operator==(const PositionSystem&) const;

    static void			initRepository(NotifierAccess* = 0);
				/*!<To be called from initGeneral with a
				    pointer to survey change notifier. */

				mDefineFactoryInClass(PositionSystem,factory);
				//!<Creates the subclasses without settings

    static void			getSystemNames(bool onlyorthogonal,uiStringSet&,
					       ObjectSet<IOPar>&);
				/*!Gets a list of coordinate systems and the
				   corresponding IOPars to create them.
				   IOPars become yours! */

    static RefMan<PositionSystem> createSystem(const IOPar&);
				//!<Creates subclass with settings
    virtual uiString		description() const			= 0;

    virtual bool		isOK() const				= 0;

    virtual bool		geographicTransformOK() const		= 0;
    virtual LatLong		toGeographicWGS84(const Coord&) const	= 0;
    virtual Coord		fromGeographicWGS84(const LatLong&) const = 0;

    static Coord		convert(const Coord&,const PositionSystem& from,
					const PositionSystem& to);
    Coord			convertFrom(const Coord&,
					const PositionSystem& from) const;

    virtual uiString		toUiString(const Coord&) const;
    virtual BufferString	toString(const Coord&,
					 bool withsystem=false) const;
				/*!<Returns string. If withsystem is turned on
				    it will start with the factory name of the
				    system, followed by a space. */
    virtual Coord		fromString(const char*) const;

    virtual bool		isOrthogonal() const			= 0;
    virtual bool		isFeet() const		{ return false; }
    virtual bool		isMeter() const		{ return false; }

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

    static const char*		sKeyFactoryName()	{ return "System name";}
    static const char*		sKeyUiName()		{ return "UI Name"; }
};


mExpClass(Basic) UnlocatedXY : public PositionSystem
{ mODTextTranslationClass(UnlocatedXY);
public:
    mDefaultFactoryInstantiation( PositionSystem,UnlocatedXY,"Unlocated XY",
				 tr("Unlocated XY") );

			UnlocatedXY();
			UnlocatedXY(const Coord&,const LatLong&);
    virtual uiString	description() const
			{ return
			   tr("Coordinate system in an undefined projection.");}

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const;
    void		setLatLongEstimate(const LatLong&,const Coord&);

    virtual LatLong	toGeographicWGS84(const Coord&) const;
			//!<Very aproximate! Be Aware!
    virtual Coord	fromGeographicWGS84(const LatLong&) const;

    virtual bool	isOK() const		{ return true; }
    virtual bool	isOrthogonal() const	{ return true; }
    virtual bool	isFeet() const		{ return isfeet_; }
    virtual bool	isMeter() const		{ return !isfeet_; }

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    const Coord&	refCoord() const { return refcoord_; }
    const LatLong&	refLatLong() const { return reflatlng_; }

private:

    bool		isfeet_;
    Coord		refcoord_;
    LatLong		reflatlng_;

    double		lngdist_;
};


}; //namespace

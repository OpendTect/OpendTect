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



/*! Base class for Coord systems, these are all two-dimensional and
    coordinates can be stored in Coord. They may use any projection, but they
    must be able to return Geographic coordinates using
    either the WGS84 datum or its own datum if applicable.
 */

mExpClass(Basic) CoordSystem : public RefCount::Referenced
{
public:

    bool			operator==(const CoordSystem&) const;

				mDefineFactoryInClass(CoordSystem,factory);
				//!<Creates the subclasses without settings

    static void			getSystemNames(bool onlyorthogonal,
					       bool onlyprojection,
					       uiStringSet&,
					       ObjectSet<IOPar>&);
				/*!Gets a list of coord systems and the
				   corresponding IOPars to create them.
				   IOPars become yours! */

    static RefMan<CoordSystem> createSystem(const IOPar&);
				//!<Creates subclass with settings
    virtual CoordSystem*	clone() const				= 0;

    virtual uiString		description() const			= 0;
    virtual uiString		summary() const				= 0;
    virtual bool		isWorthMentioning() const { return false; }

    virtual bool		isOK() const				= 0;

    virtual bool		geographicTransformOK() const		= 0;

    virtual const BufferString	getURNString() = 0;

    static Coord		convert(const Coord&,const CoordSystem& from,
					const CoordSystem& to);
    Coord			convertFrom(const Coord&,
					const CoordSystem& from) const;

    virtual uiString		toUiString(const Coord&) const;
    virtual BufferString	toString(const Coord&,
					 bool withsystem=false) const;
				/*!<Returns string. If withsystem is turned on
				    it will start with the factory name of the
				    system, followed by a space. */
    virtual Coord		fromString(const char*) const;

    virtual bool		isOrthogonal() const	= 0;
    virtual bool		isProjection() const	{ return false; }
    virtual bool		isFeet() const		{ return false; }
    virtual bool		isMeter() const		{ return false; }

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    static const char*		sKeyFactoryKey();
    static const char*		sKeyUiName();

protected:

    virtual LatLong		toGeographic(const Coord&,
					     bool wgs84) const		= 0;
    virtual Coord		fromGeographic(const LatLong&,
					       bool wgs84) const	= 0;
    virtual void		doFillPar(IOPar&) const			= 0;
    virtual bool		doUsePar(const IOPar&)			= 0;

private:

    friend class ::LatLong;
};


mExpClass(Basic) UnlocatedXY : public CoordSystem
{ mODTextTranslationClass(UnlocatedXY);
public:
    mDefaultFactoryInstantiation( CoordSystem,UnlocatedXY,"Unlocated XY",
				 tr("Unlocated XY") );

			UnlocatedXY();

    virtual CoordSystem*	clone() const;
    virtual uiString	description() const { return
			   tr("Coordinate system in an undefined projection.");}
    virtual uiString	summary() const
				{ return factoryDisplayName(); }

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const	{ return false; }

    virtual bool	isOK() const		{ return true; }
    virtual bool	isOrthogonal() const	{ return true; }
    virtual bool	isFeet() const		{ return isfeet_; }
    virtual bool	isMeter() const		{ return !isfeet_; }
    virtual const BufferString getURNString() { return BufferString::empty(); }


private:

    virtual LatLong	toGeographic(const Coord&,bool wgs84) const;
    virtual Coord	fromGeographic(const LatLong&,bool wgs84) const;
    virtual bool	doUsePar(const IOPar&);
    virtual void	doFillPar(IOPar&) const;

    bool		isfeet_;
};


mExpClass(Basic) AnchorBasedXY : public CoordSystem
{ mODTextTranslationClass(AnchorBasedXY);
public:
    mDefaultFactoryInstantiation( CoordSystem,AnchorBasedXY,"AnchorBased XY",
				 tr("Anchor Point Based XY") );

			AnchorBasedXY();
			AnchorBasedXY(const LatLong&,const Coord&);
    virtual CoordSystem* clone() const;
    virtual uiString	description() const { return
				tr("Coordinate system has an anchor point "
				    "for which Latitude/Longitude is known");}
    virtual uiString	summary() const;

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const;
    void		setLatLongEstimate(const LatLong&,const Coord&);

    virtual bool	isOK() const		{ return true; }
    virtual bool	isOrthogonal() const	{ return true; }
    virtual bool	isFeet() const		{ return isfeet_; }
    virtual bool	isMeter() const		{ return !isfeet_; }
    virtual const BufferString getURNString() { return BufferString::empty(); }


    const Coord&	refCoord() const { return refcoord_; }
    const LatLong&	refLatLong() const { return reflatlng_; }

private:

    virtual LatLong	toGeographic(const Coord&,bool wgs84) const;
			//!<Very approximate! Be Aware!
    virtual Coord	fromGeographic(const LatLong&,bool wgs84) const;

    bool		isfeet_;
    Coord		refcoord_;
    LatLong		reflatlng_;

    double		lngdist_;

    virtual bool	doUsePar(const IOPar&);
    virtual void	doFillPar(IOPar&) const;

};

}; //namespace

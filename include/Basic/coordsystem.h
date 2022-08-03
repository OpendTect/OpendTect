#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2016
________________________________________________________________________

-*/

#include "basicmod.h"
#include "sharedobject.h"

#include "bufstring.h"
#include "coord.h"
#include "factory.h"
#include "latlong.h"

namespace Coords
{

/*! Base class for Coord systems, these are all two-dimensional and
    coordinates can be stored in Coord. They may use any projection, but they
    must be able to return Geographic coordinates using
    either the WGS84 datum or its own datum if applicable.
 */

mExpClass(Basic) CoordSystem : public SharedObject
{
public:

    bool			operator==(const CoordSystem&) const;

    static void			initRepository(NotifierAccess* =nullptr);
				/*!<To be called from initGeneral with a
				    pointer to survey change notifier. */

				mDefineFactoryInClass(CoordSystem,factory);
				//!<Creates the subclasses without settings

    static void			getSystemNames(bool onlyorthogonal,
					       bool onlyprojection,
					       uiStringSet&,
					       ObjectSet<IOPar>&);
				/*!Gets a list of coord systems and the
				   corresponding IOPars to create them.
				   IOPars become yours! */

    static RefMan<CoordSystem>	createSystem(const IOPar&);
				//!<Creates subclass with settings
    virtual CoordSystem*	clone() const				= 0;
    virtual CoordSystem*	getGeodeticSystem() const
				{ return clone(); }

    virtual uiString		description() const			= 0;
    virtual BufferString	summary() const				= 0;

    virtual bool		isOK() const				= 0;

    virtual bool		geographicTransformOK() const		= 0;

    virtual BufferString	getURNString() const = 0;

    static Coord		convert(const Coord&,const CoordSystem& from,
					const CoordSystem& to);

    virtual Coord		convertFrom(const Coord&,
					const CoordSystem& from) const;
    virtual Coord		convertTo(const Coord&,
					const CoordSystem& to) const;

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
    virtual bool		isGeodetic() const	{ return false; }
    virtual bool		isWGS84() const		{ return false; }

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    static const char*		sKeyFactoryName();
    static const char*		sKeyUiName();

    static CoordSystem*		getWGS84LLSystem();
    static BufferString		sWGS84ProjDispString();

protected:
    virtual			~CoordSystem();

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
{
mODTextTranslationClass(UnlocatedXY)
public:
    mDefaultFactoryInstantiation( CoordSystem,UnlocatedXY,"Unlocated XY",
				 tr("Unlocated XY") );

			UnlocatedXY();

    CoordSystem*	clone() const override;
    uiString	description() const override
		{ return tr("Coordinate system in an undefined projection.");}
    BufferString summary() const override { return sFactoryKeyword(); }

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const override { return false; }

    bool		isOK() const override		{ return true; }
    bool		isOrthogonal() const override	{ return true; }
    bool		isFeet() const override		{ return isfeet_; }
    bool		isMeter() const override	{ return !isfeet_; }
    BufferString	getURNString() const override
					    { return BufferString::empty(); }

private:

    LatLong	toGeographic(const Coord&,bool wgs84) const override;
    Coord	fromGeographic(const LatLong&,bool wgs84) const override;
    bool	doUsePar(const IOPar&) override;
    void	doFillPar(IOPar&) const override;

    bool		isfeet_;
};


mExpClass(Basic) AnchorBasedXY : public CoordSystem
{
mODTextTranslationClass(AnchorBasedXY)
public:
    mDefaultFactoryInstantiation( CoordSystem,AnchorBasedXY,"AnchorBased XY",
				 tr("Anchor Point Based XY") );

			AnchorBasedXY();
			AnchorBasedXY(const LatLong&,const Coord&);
    CoordSystem* clone() const override;
    uiString	description() const override
		    { return tr("Coordinate system has an anchor point "
				    "for which Latitude/Longitude is known");}
    BufferString	summary() const override;

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const override;
    void		setLatLongEstimate(const LatLong&,const Coord&);

    bool		isOK() const override		{ return true; }
    bool		isOrthogonal() const override	{ return true; }
    bool		isFeet() const override		{ return isfeet_; }
    bool		isMeter() const override	{ return !isfeet_; }

    const Coord&	refCoord() const { return refcoord_; }
    const LatLong&	refLatLong() const { return reflatlng_; }
    BufferString	getURNString() const override
					    { return BufferString::empty(); }

private:

    LatLong		toGeographic(const Coord&,bool wgs84) const override;
			//!<Very approximate! Be Aware!
    Coord		fromGeographic(
				    const LatLong&,bool wgs84) const override;

    bool		isfeet_		= false;
    Coord		refcoord_	= Coord::udf();
    LatLong		reflatlng_	= LatLong::udf();

    double		lngdist_	= mUdf(double);

    virtual bool	doUsePar(const IOPar&) override;
    virtual void	doFillPar(IOPar&) const override;

};

} // namespace Coords

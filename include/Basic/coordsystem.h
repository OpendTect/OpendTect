#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool			operator ==(const CoordSystem&) const;
    bool			operator !=(const CoordSystem&) const;

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
    static RefMan<CoordSystem>	createSystem(const char* str,
					     BufferString& msgs);

    virtual CoordSystem*	clone() const				= 0;
    virtual CoordSystem*	getGeodeticSystem() const
				{ return clone(); }

    virtual uiString		description() const			= 0;
    virtual BufferString	summary() const				= 0;

    virtual bool		isOK() const				= 0;

    virtual bool		geographicTransformOK() const		= 0;

    static Coord		convert(const Coord&,const CoordSystem& from,
					const CoordSystem& to);

    virtual Coord		convertFrom(const Coord&,
					const CoordSystem& from) const;
    virtual Coord		convertTo(const Coord&,
					const CoordSystem& to) const;

    enum StringType		{ Default, URN, WKT, JSON, URL };
    virtual BufferString	toString(StringType=Default,
					 bool withsystem=false) const	= 0;
				/*!<Returns string. If withsystem is turned on
				    it will start with the factory name of the
				    system, followed by a space. */
    virtual RefMan<CoordSystem> fromString(const char*,
					   BufferString* msg=nullptr) const = 0;

    virtual bool		isOrthogonal() const			= 0;
    virtual bool		isLatLong() const			= 0;
    virtual bool		isProjection() const	{ return false; }
    virtual bool		isFeet() const		{ return false; }
    virtual bool		isMeter() const		{ return false; }
    virtual bool		isGeodetic() const	{ return false; }
    virtual bool		isWGS84() const		{ return false; }
    virtual BufferString	getUnitName() const
					{ return BufferString::empty(); }
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    static const char*		sKeyFactoryName();
    static const char*		sKeyUiName();

    static CoordSystem*		getWGS84LLSystem();
    static BufferString		sWGS84ProjDispString();

protected:
				~CoordSystem();

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
    uiString		description() const override
			{ return tr("Coordinate system in an "
				    "undefined projection."); }
    BufferString	summary() const override { return sFactoryKeyword(); }

    void		setIsFeet( bool isfeet ) { isfeet_ = isfeet; }
    bool		geographicTransformOK() const override { return false; }

    bool		isOK() const override		{ return true; }
    bool		isOrthogonal() const override	{ return false; }
    bool		isLatLong() const override	{ return false; }
    bool		isFeet() const override		{ return isfeet_; }
    bool		isMeter() const override	{ return !isfeet_; }
    BufferString	toString(StringType=Default,
				 bool withsystem=false) const override
					    { return BufferString::empty(); }
    RefMan<CoordSystem> fromString(const char*,
				   BufferString* msg=nullptr) const override
							{ return nullptr; }

private:

    LatLong		toGeographic(const Coord&,bool wgs84) const override;
    Coord		fromGeographic(const LatLong&,
				       bool wgs84) const override;
    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;

    bool		isfeet_ = false;
};


mExpClass(Basic) AnchorBasedXY : public CoordSystem
{
mODTextTranslationClass(AnchorBasedXY)
public:
    mDefaultFactoryInstantiation( CoordSystem,AnchorBasedXY,"AnchorBased XY",
				 tr("Anchor Point Based XY") );

			AnchorBasedXY();
			AnchorBasedXY(const LatLong&,const Coord&);

    CoordSystem*	clone() const override;
    uiString		description() const override
			{ return tr("Coordinate system has an anchor point "
				    "for which Latitude/Longitude is known"); }
    BufferString	summary() const override;

    void		setIsFeet( bool isfeet )	{ isfeet_ = isfeet; }
    bool		geographicTransformOK() const override;
    void		setLatLongEstimate(const LatLong&,const Coord&);

    bool		isOK() const override		{ return true; }
    bool		isOrthogonal() const override	{ return false; }
    bool		isLatLong() const override	{ return false; }
    bool		isFeet() const override		{ return isfeet_; }
    bool		isMeter() const override	{ return !isfeet_; }

    const Coord&	refCoord() const { return refcoord_; }
    const LatLong&	refLatLong() const { return reflatlng_; }
    BufferString	toString(StringType=Default,
				 bool withsystem=false) const override;
    RefMan<CoordSystem> fromString(const char*,
				   BufferString* msg=nullptr) const override;

private:

    LatLong		toGeographic(const Coord&,bool wgs84) const override;
			//!<Very approximate! Be Aware!
    Coord		fromGeographic(const LatLong&,
				       bool wgs84) const override;
    bool		doUsePar(const IOPar&) override;
    void		doFillPar(IOPar&) const override;

    bool		isfeet_		= false;
    Coord		refcoord_	= Coord::udf();
    LatLong		reflatlng_	= LatLong::udf();

    double		lngdist_	= mUdf(double);

};

} // namespace Coords

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "coord.h"
#include "trckey.h"
#include "trigonometry.h"
#include "coordsystem.h"


namespace Pick
{

/*!\brief Pick location in space,

A pick location always has a position in X, Y and Z ('pos()'). If asked, you'll
always get the corresponding TrcKey. But, the TrcKey may not be stored as
such. If this matters, try hasTrcKey(). If there is no available TrcKey, then
it will be generated from the default survey geometry.

A Location has an optional text. This has to be used as a keyed storage like:
key1'xx'key2'yy
If no text is available, you'll get an empty string.

A Location also has an optional direction:
* phi is defined as the direction's counter-clockwise angle from the x-axis
	in the x-y plane.
* theta is defined as the directions angle from the upward pointing z axis
	(i.e. opposite to survey-z-axis).
Theta and the radius are defined after the SI().zFactor is applied to the
z-coordinate.
If no direction is available, you'll get nullSphere().

 */

mExpClass(General) Location
{
public:
			Location(double x=0,double y=0,double z=0);
			Location(const Coord&,float z=0);
			Location(const Coord3&);
			Location(const Coord3& pos,const Coord3& dir);
			Location(const Coord3& pos,const Sphere& dir);
			Location(const Location&);
			~Location();

    inline bool		operator ==( const Location& oth ) const
			{ return pos_ == oth.pos_ && dir_ == oth.dir_; }
    inline bool		operator !=( const Location& oth ) const
			{ return !(*this == oth); }
    void		operator =(const Location&);

    inline bool		hasPos() const		{ return pos_.isDefined(); }
    bool		hasTrcKey() const;
    inline bool		hasDir() const		{ return !dir_.isNull(); }
    bool		hasText() const;

    inline const Coord3& pos() const		{ return pos_; }
    inline float	z() const		{ return (float)pos_.z; }
    bool		is2D() const;
    OD::GeomSystem	geomSystem() const;
    Pos::GeomID		geomID() const;
    const TrcKey&	trcKey() const;
    Pos::LineID		lineNr() const;
    Pos::TraceID	trcNr() const;
    const BinID&	binID() const;
    const Sphere&	dir() const;
    const BufferString& text() const;

    inline Location&	setPos( const Coord3& c )
			{ pos_ = c; return *this; }
    inline Location&	setPos( const Coord& c )
			{ pos_.x = c.x; pos_.y = c.y; return *this; }
    inline Location&	setPos( double x, double y, double zval )
			{ setPos( Coord3(x,y,zval) ); return *this; }
    inline Location&	setPos( const Coord& c, float zval )
			{ setPos( c.x, c.y, zval ); return *this; }
    template <class FT>
    inline Location&	setZ( FT zval )
			{ pos_.z = zval; return *this; }

    Location&		setTrcKey(const TrcKey&);
    Location&		setDir(const Sphere&);
    Location&		setDir(const Coord&);
    Location&		setLineNr(Pos::LineID);
    Location&		setTrcNr(Pos::LineID);
    Location&		setGeomID(Pos::GeomID);
    Location&		setBinID(const BinID&,bool updcoord=false);
    Location&		setGeomSystem(OD::GeomSystem,bool updfromcoord=true);

    bool		hasTextKey(const char* key) const;
    bool		getKeyedText(const char* key,BufferString&) const;
    void		setKeyedText(const char* key,const char* txt);
    void		removeTextKey(const char* key);
    Location&		setText(const char*);
			//!< make sure it's compatible with the keying system

    bool		fromString(const char*);
    void		toString(BufferString&,bool forexport=false,
				      const Coords::CoordSystem* crs=0) const;

			// renamed to 'Keyed' in post-6.0
    /* mDeprecated */ void setText(const char* key,const char* txt);
    /* mDeprecated */ void unSetText(const char* key);
    /* mDeprecated */ bool getText(const char* key,BufferString&) const;

			// will go away post-6.0
    /* mDeprecated */ void	setDip(float,float);
    /* mDeprecated */ float	inlDip() const;
    /* mDeprecated */ float	crlDip() const;

			// become protected in post-6.0
    /* mDeprecated */ Coord3		pos_;
    /* mDeprecated */ TrcKey		trckey_;
    /* mDeprecated */ Sphere		dir_;
    /* mDeprecated */ BufferString*	text_;

protected:

    bool		fndKeyTxt(const char*,BufferString*) const;

public:

    mDeprecated("Use setGeomSystem")
    Location&		setSurvID(OD::GeomSystem,bool updfromcoord=true);

    mDeprecated("Use geomSystem")
    OD::GeomSystem	survID() const;
};

} // namespace Pick


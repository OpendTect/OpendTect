#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "locationbase.h"


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

mExpClass(General) Location : public LocationBase
{
public:
			Location(const Coord3& pos,
				    const Coord3& dir =Coord3::udf(),
				    const Pos::GeomID& =Pos::GeomID::udf());
			Location(const Coord3& pos,const Sphere& dir,
				    const Pos::GeomID& =Pos::GeomID::udf());
			Location(const Coord&,double z=0.,
				    const Coord3& dir =Coord3::udf(),
				    const Pos::GeomID& =Pos::GeomID::udf());
			Location(double x=0.,double y=0.,double z=0.,
				    const Coord3& dir =Coord3::udf(),
				    const Pos::GeomID& =Pos::GeomID::udf());
			Location(const Location&);
			~Location();

    inline bool		operator ==( const Location& oth ) const
			{ return pos_ == oth.pos_ && dir_ == oth.dir_; }
    inline bool		operator !=( const Location& oth ) const
			{ return !(*this == oth); }
    void		operator =(const Location&);

    inline bool		hasDir() const		{ return !dir_.isNull(); }
    bool		hasText() const;

    const Sphere&	dir() const;
    const BufferString& text() const;

    bool		hasTextKey(const char* key) const;
    bool		getKeyedText(const char* key,BufferString&) const;
    void		setKeyedText(const char* key,const char* txt);
    void		removeTextKey(const char* key);
    Location&		setText(const char*);
    Location&		setDir(const Sphere&);
    Location&		setDir(const Coord&); //Why keep it not even has body
			//!< make sure it's compatible with the keying system

    bool		fromString(const char*);
    void		toString(BufferString&,bool forexport=false,
				      const Coords::CoordSystem* crs=0) const;

    mDeprecated("Use setKeyedText")
    void		setText(const char* key,const char* txt);
    mDeprecated("Use removeTextKey")
    void		unSetText(const char* key);
    mDeprecated("Use getKeyedText")
    bool		getText(const char* key,BufferString&) const;

    void		setDip(float,float);
    float		inlDip() const;
    float		crlDip() const;

protected:

    Sphere		dir_;
    BufferString*	text_;


    bool		fndKeyTxt(const char*,BufferString*) const;
};

} // namespace Pick

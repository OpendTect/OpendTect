#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "picklabel.h"
#include "coord.h"
#include "coordsystem.h"
#include "geomid.h"
#include "binid.h"
#include "bin2d.h"
class TrcKey;
class Sphere;


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

    mUseType( Pos,		GeomID );
    typedef GroupLabel::ID	GroupLabelID;
    typedef Pos::TraceNr_Type	trcnr_type;
    typedef trcnr_type		linenr_type;

			Location();
			Location(double x,double y,double z=0);
			Location(const Coord&,float z=0);
			Location(const Coord3&);
			Location(const Coord3&,const Sphere& direction);
			Location(const Location&);
			~Location();

    bool		operator ==(const Location&) const;
			mImplSimpleIneqOper(Location)
    Location&		operator =(const Location&);

    inline bool		hasPos() const		{ return pos_.isDefined(); }
    bool		hasTrcKey() const;
    bool		hasDir() const;
    bool		hasText() const;

    inline const Coord3& pos() const		{ return pos_; }
    inline float	z() const		{ return (float)pos_.z_; }
    bool		is2D() const;
    GeomID		geomID() const;
    const TrcKey&	trcKey() const;
    linenr_type		lineNr() const;
    trcnr_type		trcNr() const;
    BinID		binID() const;
    Bin2D		bin2D() const;
    const Sphere&	dir() const;
    const BufferString&	text() const;
    GroupLabelID	groupLabelID() const	{ return grplblid_; }

    inline Location&	setPos( const Coord3& c )
			{ pos_ = c; return *this; }
    inline Location&	setPos( const Coord& c )
			{ pos_.x_ = c.x_; pos_.y_ = c.y_; return *this; }
    inline Location&	setPos( double x, double y, double zval )
			{ setPos( Coord3(x,y,zval) ); return *this; }
    inline Location&	setPos( const Coord& c, float zval )
			{ setPos( c.x_, c.y_, zval ); return *this; }
    template <class FT>
    inline Location&	setZ( FT zval )
			{ pos_.z_ = zval; return *this; }
    inline Location&	setGroupLabelID( GroupLabelID id )
			{ grplblid_ = id; return *this; }

    Location&		setTrcKey(const TrcKey&);
    Location&		setDir(const Sphere&);
    Location&		setDir(const Coord&);
    Location&		setLineNr(linenr_type);
    Location&		setTrcNr(trcnr_type);
    Location&		setGeomID(GeomID);
    Location&		setPos(const BinID&,bool updcoord=false);
    Location&		setPos(GeomID,trcnr_type,bool updcoord=false);

    bool		hasTextKey(const char* key) const;
    bool		getKeyedText(const char* key,BufferString&) const;
    void		setKeyedText(const char* key,const char* txt);
    void		removeTextKey(const char* key);
    Location&		setText(const char*);
			//!< make sure it's compatible with the keying system

    bool		fromString(const char*);
    void		toString(BufferString&,bool forexport=false,
					const Coords::CoordSystem* crs=0) const;

    static const Location& udf();
    static Location&	dummy();
    bool		isUdf() const		{ return *this == udf(); }
    void		setUdf()		{ *this = udf(); }

protected:

    Coord3		pos_;
    TrcKey*		trckey_;
    Sphere*		dir_;
    GroupLabelID	grplblid_;
    BufferString*	text_;

    void		setTK(const TrcKey*);
    void		setD(const Sphere*);
    bool		fndKeyTxt(const char*,BufferString*) const;
};


} // namespace Pick

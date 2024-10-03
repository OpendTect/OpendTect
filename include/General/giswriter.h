#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "coordsystem.h"
#include "draw.h"
#include "factory.h"

class od_ostream;
namespace Pick { class Set; }


namespace GIS
{

    enum class FeatureType { Undefined, Point, LineString, Polygon,
			     MultiPoint, MultiLineString, MultiPolygon };
			   mDeclareNameSpaceEnumUtils(General,FeatureType);


mExpClass(General) Property : public NamedObject
{
public:
				Property();
				Property(const Property&);
				~Property();

    Property&		operator =(const Property&);
    Property&		setType(const FeatureType&);

    bool		isPoint() const;
    bool		isLine() const;
    bool		isPolygon() const;
    bool		isMulti() const;

    FeatureType		type_		= FeatureType::Undefined;
    OD::Color		color_		= OD::Color::NoColor();
    int			pixsize_	= 2;
    OD::LineStyle	linestyle_;
    bool		dofill_		= false;
    OD::Color		fillcolor_	= OD::Color::NoColor();
    BufferString	iconnm_;
    BufferString	nmkeystr_	= "name";

};


mExpClass(General) Writer
{
mODTextTranslationClass(Writer)
public:
    mDefineFactoryInClass(Writer,factory);

    virtual			~Writer();

    virtual Writer&		setInputCoordSys(const Coords::CoordSystem*);
    virtual Writer&		setSurveyName(const char*);
				//!< before open()
    virtual Writer&		setElemName(const char*);
				//!< before open()
    virtual Writer&		setStream(const char* fnm,
					  bool useexisting=false)	= 0;
    virtual Writer&		setDescription(const char*);
    virtual Writer&		setProperties(const Property&);

    virtual bool		isOK() const;
    virtual BufferString	getExtension() const			= 0;
    virtual void		getDefaultProperties(const FeatureType&,
						     Property&) const	= 0;

    uiString			successMsg();
    uiString			errMsg()	{ return errmsg_; }

				// (single) Point
    virtual bool		writePoint(const Coord&,
					   const char* nm=nullptr)	= 0;
    virtual bool		writePoint(const Coord3&,
					   const char* nm=nullptr)	= 0;
    virtual bool		writePoint(const LatLong&,
					   const char* nm=nullptr,
					   double z=0.)			= 0;

				// (single) Line
    virtual bool		writeLine(const TypeSet<Coord>&,
					  const char* nm=nullptr)	= 0;
    virtual bool		writeLine(const TypeSet<Coord3>&,
					  const char* nm=nullptr)	= 0;
    virtual bool		writeLine(const Pick::Set&)		= 0;

				// (single) Polygon
    virtual bool		writePolygon(const TypeSet<Coord>&,
					     const char* nm=nullptr)	= 0;
    virtual bool		writePolygon(const TypeSet<Coord3>&,
					     const char* nm=nullptr)	= 0;
    virtual bool		writePolygon(const Pick::Set&)		= 0;

				// PointSet
    virtual bool		writePoints(const TypeSet<Coord>&,
					    const char* nm=nullptr)	= 0;
    virtual bool		writePoints(const TypeSet<Coord3>&,
					    const char* nm=nullptr)	= 0;
    virtual bool		writePoints(const Pick::Set&)		= 0;

				// (Multiple) Lines
    virtual bool		writeLines(const Pick::Set&)	{ return false;}

				// (Multiple) Polygons
    virtual bool		writePolygons(const Pick::Set&) { return false;}

protected:
				Writer();

    virtual bool		open(const char* fnm,bool useexisting)	= 0;
    virtual bool		close();
    od_ostream&			strm() { return *strm_; }
    const od_ostream&		strm() const { return *strm_; }
    bool			doLineCheck(int sz);
    bool			doPolygonCheck(int sz);
    const Coords::CoordSystem*	getOutputCRS() const;

    od_ostream*				strm_	= nullptr;
    ConstRefMan<Coords::CoordSystem>	inpcrs_;

    Property			properties_;
    uiString			errmsg_;

private:
    ConstRefMan<Coords::CoordSystem>	coordsys_;
};

} // namespace GIS

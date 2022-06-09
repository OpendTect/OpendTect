#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : March 2021
-*/

#include "generalmod.h"
#include "uistring.h"
#include "od_ostream.h"
#include "factory.h"
#include "pickset.h"
#include "coordsystem.h"


mExpClass(General) GISWriter
{ mODTextTranslationClass(GISWriter)
public:
    mDefineFactoryInClass(GISWriter,factory);

    mExpClass(General) Property
    {
	public:
				Property();
				~Property();

	OD::Color		color_		= OD::Color::Black();
	int			width_		= 2;
	BufferString		iconnm_		= "NONE";
	BufferString		stlnm_		= "NONE";
	int			xpixoffs_	= 20;
	BufferString		objnm_		= "NONE";
	BufferString		coordysynm_	= "NONE";
	BufferString		nmkeystr_	= "name";

	Property&		operator =(const Property& oth);
    };

    virtual			~GISWriter();

    bool			isOK() const { return strm_ && strm_->isOK(); }
    od_ostream&			strm() { return *strm_; }
    const od_ostream&		strm() const { return *strm_; }

    virtual void		setStream(const BufferString& fnm) = 0;

    RefObjectSet<const Pick::Set>		picks_;

    virtual bool		close();
    virtual uiString		errMsg() const = 0;
    virtual BufferString	getExtension() = 0;

    virtual bool		writePoints(const TypeSet<Coord>&,
					const BufferStringSet& nms) = 0;
    virtual bool		writePoint(const Coord&,const char* nm=0) = 0;
    virtual bool		writePoint(const LatLong&,const char* nm=0) = 0;
    virtual bool		writePoint(
				    const RefObjectSet<const Pick::Set>&) = 0;
    virtual bool		writeLine(const TypeSet<Coord>&,
					  const char* nm=nullptr) = 0;
    virtual bool		writeLine(
				    const RefObjectSet<const Pick::Set>&) = 0;
    virtual bool		writePolygon(const TypeSet<Coord>&,
					     const char* nm=nullptr) = 0;
    virtual bool		writePolygon(const TypeSet<Coord3>&,
					     const char* nm=nullptr) = 0;
    virtual bool		writePolygon(
				    const RefObjectSet<const Pick::Set>&) = 0;
    void			setCoordSys(Coords::CoordSystem* crs)
				{ coordsys_ = crs; }
    ConstRefMan<Coords::CoordSystem>	getCoordSys() { return coordsys_; }
    void			coordConverter( TypeSet<Coord>& crdset );
    void			coordConverter( TypeSet<Coord3>& crdset );
    uiString			successMsg();
    uiString			errMsg();

    void			setProperties(const GISWriter::Property&);

protected:
					GISWriter();
    od_ostream*				strm_	= nullptr;
    ConstRefMan<Coords::CoordSystem>	coordsys_;
    ConstRefMan<Coords::CoordSystem>	inpcrs_;

    Property				properties_;
};

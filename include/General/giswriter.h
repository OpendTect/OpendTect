#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "generalmod.h"
#include "uistring.h"
#include "od_ostream.h"
#include "factory.h"
#include "pickset.h"
#include "coordsystem.h"


typedef TypeSet<Coord>		coord2dset;
typedef TypeSet<Coord3d>	coord3dset;
typedef ObjectSet<Pick::Set>	pickset;

mExpClass(General) GISWriter
{
public:
    mDefineFactoryInClass(GISWriter, factory);
    struct Property
    {
	Color		    color_ = Color::Black();
	int		    width_ = 2;
	BufferString	    iconnm_ = "NONE";
	BufferString	    stlnm_ = "NONE";
	int		    xpixoffs_ = 20;
	BufferString	    objnm_ = "NONE";
	BufferString	    coordysynm_ = "NONE";
	BufferString	    nmkeystr_ = "name";
	inline Property& operator =(const Property& oth)
	{
	    color_	= oth.color_;
	    width_	= oth.width_;
	    iconnm_	= oth.iconnm_;
	    stlnm_	= oth.stlnm_;
	    xpixoffs_	= oth.xpixoffs_;
	    objnm_	= oth.objnm_;
	    coordysynm_ = oth.coordysynm_;
	    nmkeystr_	= oth.nmkeystr_;

	    return *this;
	}
    };

    bool		    isOK() const { return strm_ && strm_->isOK(); }
    od_ostream&		    strm() { return *strm_; }
    const od_ostream&	    strm() const { return *strm_; }

    virtual void	    setStream(const BufferString& fnm) = 0;

    pickset		    picks_;


    virtual void	    close() = 0;
    virtual uiString	    errMsg() const = 0;
    virtual BufferString    getExtension() = 0;

    virtual void	    writePoints(const coord2dset&,
					const BufferStringSet& nms) = 0;
    virtual void	    writePoint(const Coord&, const char*nm = 0) = 0;
    virtual void	    writePoint(const pickset&) = 0;
    virtual void	    writeLine(const coord2dset&, const char*nm = 0) = 0;
    virtual void	    writeLine(const pickset&) = 0;
    virtual void	    writePolygon(const coord2dset&,
							const char*nm = 0) = 0;
    virtual void	    writePolygon(const coord3dset&,
							const char*nm = 0) = 0;
    virtual void	    writePolygon(const pickset&) = 0;
    void		    setProperties(const Property& properties);
    void		    setCoordSys(Coords::CoordSystem* crs)
			    { coordsys_ = crs; }
    ConstRefMan<Coords::CoordSystem>   getCoordSys() { return coordsys_; }
    void		    coordConverter( TypeSet<Coord>& crdset );
    void		    coordConverter( TypeSet<Coord3d>& crdset );

protected:
					    GISWriter();
    od_ostream*				    strm_;
    bool				    ispropset_ = false;
    Property				    properties_;
    ConstRefMan<Coords::CoordSystem>	    coordsys_;
    ConstRefMan<Coords::CoordSystem>	    inpcrs_;
};

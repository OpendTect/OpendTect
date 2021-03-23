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


typedef TypeSet<Coord>			coord2dset;
typedef TypeSet<Coord3>			coord3dset;
typedef ObjectSet<const Pick::Set>	pickset;

mExpClass(General) GISWriter
{ mODTextTranslationClass(GISWriter)
public:
    mDefineFactoryInClass(GISWriter,factory);

    mExpClass(General) Property
    {
	public:
					Property();
					~Property();

	OD::Color	    color_	= OD::Color::Black();
	int		    width_	= 2;
	BufferString	    iconnm_	= BufferString("NONE");
	BufferString	    stlnm_	= BufferString("NONE");
	int		    xpixoffs_	= 20;
	BufferString	    objnm_	= BufferString("NONE");
	BufferString	    coordysynm_ = BufferString("NONE");
	BufferString	    nmkeystr_	= BufferString("name");

	Property&	    operator =(const Property& oth);
    };

    virtual		    ~GISWriter();

    bool		    isOK() const { return strm_ && strm_->isOK(); }
    od_ostream&		    strm() { return *strm_; }
    const od_ostream&	    strm() const { return *strm_; }

    virtual void	    setStream(const BufferString& fnm) = 0;

    pickset		    picks_;

    virtual bool	    close();
    virtual uiString	    errMsg() const = 0;
    virtual BufferString    getExtension() = 0;

    virtual bool	    writePoints(const coord2dset&,
					const BufferStringSet& nms) = 0;
    virtual bool	    writePoint(const Coord&, const char*nm = 0) = 0;
    virtual bool	    writePoint(const pickset&) = 0;
    virtual bool	    writeLine(const coord2dset&,
						const char*nm = nullptr) = 0;
    virtual bool	    writeLine(const pickset&) = 0;
    virtual bool	    writePolygon(const coord2dset&,
						const char*nm = nullptr) = 0;
    virtual bool	    writePolygon(const coord3dset&,
						const char*nm = nullptr) = 0;
    virtual bool	    writePolygon(const pickset&) = 0;
    void		    setCoordSys(Coords::CoordSystem* crs)
			    { coordsys_ = crs; }
    ConstRefMan<Coords::CoordSystem>   getCoordSys() { return coordsys_; }
    void		    coordConverter( TypeSet<Coord>& crdset );
    void		    coordConverter( TypeSet<Coord3>& crdset );
    uiString		    successMsg();
    uiString		    errMsg();

protected:
					    GISWriter();
    od_ostream*				    strm_	= nullptr;
    ConstRefMan<Coords::CoordSystem>	    coordsys_;
    ConstRefMan<Coords::CoordSystem>	    inpcrs_;

    Property				    properties_;
};

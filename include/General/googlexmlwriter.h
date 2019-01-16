#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/

#include "giswriter.h"
#include "factory.h"

class LatLong;
class SurveyInfo;

namespace ODGoogle
{
class XMLItem;

/*!
\brief XML Writer.
*/

mExpClass(General) KMLWriter : public GISWriter
{ mODTextTranslationClass(KMLWriter);
public:
    mDefaultFactoryInstantiation( GISWriter, ODGoogle::KMLWriter, "KML",
							toUiString("KML") );

			KMLWriter() {}
			~KMLWriter()		{ close(); }

    uiString		errMsg() const		{ return errmsg_; }

    void		setElemName( const char* nm ) //!< before open()
						{ elemnm_ = nm; }
    void		setSurveyName( const char* nm ) //!< before open()
						{ survnm_ = nm; }
    void		setStream(const BufferString& fnm);
    void		setDesc(const BufferString& desc) { desc_ = desc; }

    void		close();



    virtual void	writePoints(const coord2dset&,
					    const BufferStringSet& nms);
    virtual void	writeLine(const coord2dset&, const char* nm = 0);
    virtual void	writePoint(const Coord&, const char* nm = 0);
    virtual void	writePolygon(const coord2dset&,
						const char* nm = 0);
    virtual void	writePolygon(const coord3dset&,
						const char* nm = 0);
    virtual void	writeLine(const pickset&);
    virtual void	writePoint(const pickset&);
    virtual void	writePolygon(const pickset&);
    BufferString	getExtension() { return BufferString("kml"); }
protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    uiString		errmsg_;
    BufferString	desc_;
    bool		open(const char* fnm);
    void		putPlaceMark(const Coord&,const char* nm);
    void		putPlaceMark(const LatLong&,const char* nm);
    void		putLine(const TypeSet<Coord>&,const char* nm);

    void		putPolyStyle();
    void		putPoly(const TypeSet<Coord3d>&,const char* nm=0);
    void		putIconStyles();
};


} // namespace ODGoogle

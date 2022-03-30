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

			KMLWriter();
			~KMLWriter();

    uiString	errMsg() const override			{ return errmsg_; }

    void	setElemName( const char* nm ) //!< before open()
						{ elemnm_ = nm; }
    void	setSurveyName( const char* nm ) //!< before open()
						{ survnm_ = nm; }
    void	setStream(const BufferString& fnm) override;
    void	setDesc(const BufferString& desc) { desc_ = desc; }

    bool	close() override;

    bool	writePoints(const coord2dset&,
					const BufferStringSet& nms) override;
    bool	writeLine(const coord2dset&, const char* nm=nullptr) override;
    bool	writePoint(const Coord&, const char* nm=nullptr) override;
    bool	writePolygon(const coord2dset&,const char* nm=nullptr) override;
    bool	writePolygon(const coord3dset&,const char* nm=nullptr) override;
    bool	writeLine(const pickset&) override;
    bool	writePoint(const pickset&) override;
    bool	writePolygon(const pickset&) override;
    BufferString getExtension() override	{ return BufferString("kml"); }
protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    uiString		errmsg_;
    BufferString	desc_;
    bool		open(const char* fnm);

    bool		putPlaceMark(const Coord&,const char* nm);
    bool		putPlaceMark(const LatLong&,const char* nm);
    bool		putLine(const TypeSet<Coord>&,const char* nm);
    bool		putPolyStyle();
    bool		putPoly(const TypeSet<Coord3>&,const char* nm=nullptr);
    bool		putIconStyles();
};


} // namespace ODGoogle

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
    mDefaultFactoryInstantiation(GISWriter,ODGoogle::KMLWriter, "KML",
				 toUiString("KML"));

			KMLWriter();
			~KMLWriter();

    BufferString	getExtension() override	{ return BufferString("kml"); }
    uiString		errMsg() const override		{ return errmsg_; }

    void		setElemName( const char* nm ) //!< before open()
			{ elemnm_ = nm; }
    void		setSurveyName( const char* nm ) //!< before open()
			{ survnm_ = nm; }
    void		setStream(const BufferString& fnm) override;
    void		setDesc(const BufferString& desc) { desc_ = desc; }

    bool		close() override;

    bool		writePoints(const TypeSet<Coord>&,
					const BufferStringSet& nms) override;
    bool		writeLine(const TypeSet<Coord>&,
			const char* nm=nullptr) override;
    bool		writePoint(const Coord&,
				   const char* nm=nullptr) override;
    bool		writePoint(const LatLong&,
				   const char* nm=nullptr) override;
    bool		writePolygon(const TypeSet<Coord>&,
				     const char* nm=nullptr) override;
    bool		writePolygon(const TypeSet<Coord3>&,
				     const char* nm=nullptr) override;
    bool		writeLine(
				const RefObjectSet<const Pick::Set>&) override;
    bool		writePoint(
				const RefObjectSet<const Pick::Set>&) override;
    bool		writePolygon(
				const RefObjectSet<const Pick::Set>&) override;

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

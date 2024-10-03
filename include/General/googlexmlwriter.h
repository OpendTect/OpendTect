#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "giswriter.h"

class LatLong;

namespace ODGoogle
{

/*!
\brief XML Writer.
*/

mExpClass(General) KMLWriter : public GIS::Writer
{
mODTextTranslationClass(KMLWriter);
public:
    mDefaultFactoryInstantiation(GIS::Writer,ODGoogle::KMLWriter, "KML",
				 ::toUiString("KML"));

			~KMLWriter();

private:
			KMLWriter();
			mOD_DisableCopy(KMLWriter);

    GIS::Writer&	setSurveyName(const char*) override;
    GIS::Writer&	setElemName(const char*) override;
    GIS::Writer&	setStream(const char*,bool useexisting) override;
    GIS::Writer&	setDescription(const char*) override;
    bool		open(const char* fnm,bool useexisting) override;
    bool		close() override;

    BufferString	getExtension() const override
			{ return BufferString("kml"); }
    void		getDefaultProperties(const GIS::FeatureType&,
					     GIS::Property&) const override;

    bool		writePoint(const Coord&,
				   const char* nm=nullptr) override;
    bool		writePoint(const Coord3&,
				   const char* nm=nullptr) override;
    bool		writePoint(const LatLong&,
				   const char* nm=nullptr,
				   double z=0.f) override;

    bool		writeLine(const TypeSet<Coord>&,
				  const char* nm=nullptr) override;
    bool		writeLine(const TypeSet<Coord3>&,
				  const char* nm=nullptr) override;
    bool		writeLine(const Pick::Set&) override;

    bool		writePolygon(const TypeSet<Coord>&,
				     const char* nm=nullptr) override;
    bool		writePolygon(const TypeSet<Coord3>&,
				     const char* nm=nullptr) override;
    bool		writePolygon(const Pick::Set&) override;

    bool		writePoints(const TypeSet<Coord>&,
				    const char* nm=nullptr) override;
    bool		writePoints(const TypeSet<Coord3>&,
				    const char* nm=nullptr) override;
    bool		writePoints(const Pick::Set&) override;

    bool		writeLines(const Pick::Set&) override;
    bool		writePolygons(const Pick::Set&) override;

    bool		putPlaceMark(const Coord&,const char* nm);
    bool		putPlaceMark(const Coord3&,const char* nm);
    bool		putPlaceMark(const LatLong&,double z,const char* nm);
    bool		putLine(const TypeSet<Coord>&,const char* nm);
    bool		putLine(const TypeSet<Coord3>&,const char* nm,
				bool hasdepths=true);
    bool		putPolyStyle();
    bool		putPoly(const TypeSet<Coord>&,const char* nm);
    bool		putPoly(const TypeSet<Coord3>&,const char* nm,
				bool hasdepths=true);
    bool		putIconStyles();
    bool		putFolder(const char* nm);
    bool		closeFolder();

    BufferString	elemnm_;
    BufferString	survnm_;
    BufferString	desc_;
    bool		folderopen_ = false;
    int			stlidx_ = -1;
};

} // namespace ODGoogle

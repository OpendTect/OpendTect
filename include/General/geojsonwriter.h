#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		October 2018
________________________________________________________________________

*/

#include "generalmod.h"
#include "uistring.h"
#include "enums.h"
#include "geojson.h"
#include "giswriter.h"
#include "factory.h"


class LatLong;
class SurveyInfo;
class od_ostream;


class XMLItem;

/*!
\brief XML Writer.
*/

mExpClass(General) GeoJSONWriter : public GISWriter
{ mODTextTranslationClass(GeoJSONWriter);
public:
    mDefaultFactoryInstantiation( GISWriter, GeoJSONWriter, "GeoJSON",
						    toUiString("GeoJSON") );

			GeoJSONWriter() {}
			~GeoJSONWriter() { errmsg_.setEmpty(); close(); }


    uiString		errMsg() const		{ return errmsg_; }
    void		setStream(const BufferString&);

    void		setElemName(const char* nm) //!< before open()
						{ elemnm_ = nm; }
    void		setSurveyName(const char* nm) //!< before open()
						{ survnm_ = nm; }

    void		close();

    virtual void	writePoints(const coord2dset&,
					const BufferStringSet& nms);
    virtual void	writePoint(const pickset&);
    virtual void	writePoint(const Coord&, const char* nm=0);
    virtual void	writeLine(const coord2dset&,const char* nm=0);
    virtual void	writeLine(const pickset&);
    virtual void	writePolygon(const coord2dset&,const char*nm=0);
    virtual void	writePolygon(const coord3dset&,const char*nm=0);
    virtual void	writePolygon(const pickset&);
    BufferString	getExtension() { return BufferString("geojson"); }


protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    uiString		errmsg_;
    OD::GeoJsonTree*	geojsontree_;
    bool		open(const char* fnm);
    void		writeGeometry(BufferString,const coord2dset&,
							const BufferStringSet&);
    void		writeGeometry(BufferString,const coord3dset&,
							const BufferStringSet&);
    void		writeGeometry(BufferString,const pickset&);
};

#ifndef welldata_h
#define welldata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welldata.h,v 1.6 2004-01-26 12:25:58 nanne Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "position.h"
#include "uidobj.h"

namespace Well
{

class Track;
class LogSet;
class D2TModel;
class Marker;

class Info : public ::UserIDObject
{
public:

			Info( const char* nm )
			    : ::UserIDObject(nm), surfaceelev(0)	{}

    BufferString	uwid;
    BufferString	oper;
    BufferString	state;
    BufferString	county;

    Coord		surfacecoord;
    float		surfaceelev;

    static const char*	sKeyuwid;
    static const char*	sKeyoper;
    static const char*	sKeystate;
    static const char*	sKeycounty;
    static const char*	sKeycoord;
    static const char*	sKeyelev;

};


class Data
{
public:

				Data(const char* nm=0);
				~Data();

    const char*			name() const		{ return info_.name(); }
    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }
    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }
    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }
    const ObjectSet<Marker>&	markers() const		{ return markers_; }
    ObjectSet<Marker>&		markers()		{ return markers_; }
    const D2TModel*		d2TModel() const	{ return d2tmodel_; }
    D2TModel*			d2TModel()		{ return d2tmodel_; }
    void			setD2TModel(D2TModel*);	//!< becomes mine

protected:

    Info			info_;
    Track&			track_;
    LogSet&			logs_;
    D2TModel*			d2tmodel_;
    ObjectSet<Marker>		markers_;

};

}; // namespace Well

#endif

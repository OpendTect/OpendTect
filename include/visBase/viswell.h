#ifndef viswell_h
#define viswell_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.h,v 1.9 2004-05-24 16:37:40 bert Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class Color;
class Coord3;
class Coord3Value;
class IOPar;
class LineStyle;
class SoPlaneWellLog;
template <class T> class Interval;

class SoSwitch;

namespace visBase
{
class DrawStyle;
class PolyLine;
class DataObjectGroup;
class Text;
class Transformation;

/*! \brief 
Base class for well display
*/

class Well : public VisualObjectImpl
{
public:
    static Well*		create()
    				mCreateDataObj(Well);

    void			setTrack(const TypeSet<Coord3>&);

    void			setLineStyle(const LineStyle&);
    const LineStyle&		lineStyle() const;

    void			setWellName(const char*,const Coord3&);
    void			showWellName(bool);
    bool			wellNameShown() const;

    void			addMarker(const Coord3&,const Color&,
	    				  const char*);
    void			removeAllMarkers();
    void			setMarkerScreenSize(int);
    int				markerScreenSize() const;
    bool			canShowMarkers() const;
    void			showMarkers(bool);
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;

    void			setLogData(const TypeSet<Coord3Value>&,
	    			           const char*,const Interval<float>&,
					   bool,int);
    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int);
    int				logWidth() const;
    void			showLogs(bool);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;


    void			setTransformation(visBase::Transformation*);
    visBase::Transformation*	getTransformation();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar& par);

    static const char*		linestylestr;
    static const char*		showwellnmstr;
    static const char*		showmarkerstr;
    static const char*		markerszstr;
    static const char*		showmarknmstr;
    static const char*		showlogsstr;
    static const char*		showlognmstr;

protected:
    				~Well();

    PolyLine*			track;
    DrawStyle*			drawstyle;
    Text*			welltxt;
    DataObjectGroup*		markergroup;
    SoSwitch*			markernmsw;
    DataObjectGroup*		markernames;
    SoPlaneWellLog*		log;
    Transformation*		transformation;

    bool			showmarkers;
    int				markersize;
};


};

#endif

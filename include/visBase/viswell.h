#ifndef viswell_h
#define viswell_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.h,v 1.1 2003-10-17 14:58:15 nanne Exp $
________________________________________________________________________

-*/


#include "visobject.h"

class Color;
class Coord3;
class IOPar;
class LineStyle;
class SoPlaneWellLog;

namespace visBase
{
class DrawStyle;
class PolyLine;
class SceneObjectGroup;
class Text;
class Transformation;

/*! \brief 

*/

class Well : public VisualObjectImpl
{
public:
    static Well*		create()
    				mCreateDataObj(Well);

    void			setTrack(const TypeSet<Coord3>&);

    void			setLineStyle(const LineStyle&);
    const LineStyle&		lineStyle() const;

    void			setWellText(const char*,const Coord3&);
    void			showWellText(bool);
    bool			isWellTextShown() const;

    void			addMarker(const Coord3&,const Color&);
    void			removeAllMarkers();
    void			setMarkerScale(const Coord3&);
    void			setMarkerSize(int);
    void			showMarkers(bool);
    bool			markersShown() const;

    void			setLog(const TypeSet<Coord3>&,
	    			       const TypeSet<float>&,int);
    const Color&		logColor(int) const;
    void			setLogColor(const Color&,int);
    void			showLogs(bool);
    bool			logsShown() const;


    void			setTransformation(visBase::Transformation*);
    visBase::Transformation*	getTransformation();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar& par);

    static const char*		linestylestr;
    static const char*		showwellnmstr;
    static const char*		showmarkerstr;

protected:
    				~Well();

    PolyLine*			track;
    DrawStyle*			drawstyle;
    Text*			welltxt;
    SceneObjectGroup*		markergroup;
    SoPlaneWellLog*		log;
};


};

#endif

#ifndef viswell_h
#define viswell_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: viswell.h,v 1.42 2012/02/08 21:15:28 cvsnanne Exp $
________________________________________________________________________

-*/


#include "color.h"
#include "fontdata.h"
#include "ranges.h"
#include "scaler.h"
#include "visobject.h"

class Coord3;
class Coord3Value;
class IOPar;
class LineStyle;
class TaskRunner;
class VisColorTab;
class ZAxisTransform;
class SoPlaneWellLog;
template <class T> class Interval;

class SoSwitch;

namespace visBase
{
class DrawStyle;
class PolyLineBase;
class DataObjectGroup;
class Text2;
class Transformation;

/*! \brief 
Base class for well display
*/

mClass Well : public VisualObjectImpl
{
public:

    static Well*		create()
    				mCreateDataObj(Well);

    mStruct BasicParams
    {
				BasicParams(){}
	const char* 		name_;
	Color 			col_;    
	int 			size_;    
    };
    
    //Well
    mStruct TrackParams : public BasicParams
    {
				TrackParams()
				{}
	Coord3* 		toppos_;
	Coord3* 		botpos_;
	bool 			isdispabove_;
	bool 			isdispbelow_;
	FontData		font_;
    };

    void			setTrack(const TypeSet<Coord3>&);
    void			setWellName(const TrackParams&);
    void			showWellTopName(bool);
    void			showWellBotName(bool);
    bool			wellTopNameShown() const;
    bool			wellBotNameShown() const;

    
    //Markers
    mStruct MarkerParams : public BasicParams
    {
				MarkerParams()
				{}
	int			shapeint_; 
	int			cylinderheight_; 
	bool 			issinglecol_; 
	bool 			issamenmcol_; 
	FontData		font_;
	Color			namecol_;			
	Coord3* 		pos_;
    };

    void			addMarker(const MarkerParams&);
    bool			canShowMarkers() const;
    void			showMarkers(bool);
    int				markerScreenSize() const;
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;
    void			removeAllMarkers();
    void			setMarkerScreenSize(int);
    void			setLogConstantSize(bool);
    bool			logConstantSize() const;
    float			constantLogSizeFactor() const;

    //logs
    mStruct LogParams : public BasicParams
    {
				LogParams()
				{}
	float               	cliprate_;
	bool			isdatarange_;
	bool			islinedisplayed_;
	bool                	islogarithmic_;
	bool  			issinglcol_;
	bool  			iswelllog_;
	bool 			isleftfilled_;
	bool 			isrightfilled_;
	bool			isblock_;
	int                 	logwidth_;
	int                 	logidx_;
	int                 	lognr_;
	Interval<float> 	range_;
	Interval<float> 	valrange_;
	bool 			sclog_; 
	bool			iscoltabflipped_;

	int                 	filllogidx_;
	const char*        	fillname_;
	Interval<float>     	fillrange_;
	Interval<float> 	valfillrange_;
	const char* 		seqname_;

	int                 	repeat_;
	float               	ovlap_;
	Color               	seiscolor_;
    };

    const LineStyle&		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    void 			initializeData(const LogParams&,int);
    void			setLogData(const TypeSet<Coord3Value>&, 
					   const LogParams&);
    void			setFilledLogData(const TypeSet<Coord3Value>&, 
					   	 const LogParams&);
    float 			getValue(const TypeSet<Coord3Value>&,int,bool,
	    				 const LinScaler&) const;
    Coord3 			getPos(const TypeSet<Coord3Value>&,int) const;
    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    const Color&		logFillColor(int) const;
    void			clearLog(int);
    void			setLogLineDisplayed(bool,int);
    bool			logLineDisplayed(int) const;
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int,int);
    int				logWidth() const;
    void			showLogs(bool);
    void			showLog(bool,int);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const; 
    void			setLogStyle(bool,int);
    void			setLogFill(bool,int);
    void			setLogBlock(bool,int);
    void			setOverlapp(float,int);
    void			setRepeat(int);
    void			removeLogs();
    void			hideUnwantedLogs(int,int);
    void			showOneLog(bool,int,int);
    void 			setTrackProperties(Color&,int);
    void			setLogFillColorTab(const LogParams&,int);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar& par);
    int				markersize_;
    
    static const char*		linestylestr();
    static const char*		showwelltopnmstr();
    static const char*		showwellbotnmstr();
    static const char*		showmarkerstr();
    static const char*		markerszstr();
    static const char*		showmarknmstr();
    static const char*		showlogsstr();
    static const char*		showlognmstr();
    static const char*		logwidthstr();

protected:
    				~Well();

    PolyLineBase*		track_;
    DrawStyle*			drawstyle_;
    Text2*			welltoptxt_;
    Text2*			wellbottxt_;
    DataObjectGroup*		markergroup_;
    SoSwitch*			markernmswitch_;
    DataObjectGroup*		markernames_;
    SoSwitch*			lognmswitch_;
    Text2*			lognmleft_;
    Text2*			lognmright_;
    const mVisTrans*		transformation_;

    bool			showmarkers_;
    bool			showlogs_;
    float			constantlogsizefac_;
    
    ObjectSet<SoPlaneWellLog>	log_;
    ZAxisTransform*		zaxistransform_;
};


};

#endif

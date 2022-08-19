#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "color.h"
#include "fontdata.h"
#include "ranges.h"
#include "scaler.h"
#include "visobject.h"

class TaskRunner;
class VisColorTab;
class ZAxisTransform;

namespace OD { class LineStyle; }

namespace osgGeo { class WellLog; }

namespace visBase
{

class PolyLine3D;
class PolyLine;
class PolyLineBase;
class Text2;
class Text;
class Transformation;
class MarkerSet;


/*! \brief Base class for well display */

mExpClass(visBase) Well : public VisualObjectImpl
{
public:

    typedef std::pair<Coord3,float> Coord3Value;

    static Well*		create()
				mCreateDataObj(Well);

    enum			Side { Left=0, Right, Center };

    enum			LogStyle { Welllog, Seismic, Logtube };

    mStruct(visBase) BasicParams
    {
				BasicParams(){}
	BufferString		name_;
	OD::Color		col_;
	int			size_;
    };

    //Well
    mStruct(visBase) TrackParams : public BasicParams
    {
				TrackParams()
				{}
	const Coord3*		toppos_;
	const Coord3*		botpos_;
	bool			isdispabove_;
	bool			isdispbelow_;
	FontData		font_;
	bool			nmsizedynamic_;
    };

    void			setTrack(const TypeSet<Coord3>&);
    void			setWellName(const TrackParams&);
    void			showWellTopName(bool);
    void			showWellBotName(bool);
    bool			wellTopNameShown() const;
    bool			wellBotNameShown() const;
    const PolyLine*		getTrack() const { return track_; }

    //Markers
    mStruct(visBase) MarkerParams : public BasicParams
    {
				MarkerParams()
				{}
	int			shapeint_;
	int			cylinderheight_;
	FontData		font_;
	OD::Color		namecol_;
	const Coord3*		pos_;
	bool			nmsizedynamic_;
    };

    void			setMarkerSetParams(const MarkerParams&);
    void			addMarker(const MarkerParams&);

    bool			canShowMarkers() const;
    void			showMarkers(bool);
    int				markerScreenSize() const;
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;
    const visBase::Text2*	getMarkerNames() const { return markernames_; }
    void			removeAllMarkers();
    void			setMarkerScreenSize(int);

    //logs
    mStruct(visBase) LogParams : public BasicParams
    {
				LogParams()
				{}
	float			cliprate_;
	bool			isdatarange_;
	bool			islinedisplayed_;
	bool			islogarithmic_;
	bool			issinglcol_;
	bool			isleftfilled_;
	bool			isrightfilled_;
	bool			isblock_;
	int			logwidth_;
	int			logidx_;
	Well::Side              side_;
	Interval<float>		range_;
	Interval<float>		valrange_;
	bool			sclog_;
	bool			iscoltabflipped_;

	int			filllogidx_;
	const char*		fillname_;
	Interval<float>		fillrange_;
	Interval<float>		valfillrange_;
	const char*		seqname_;

	int			repeat_;
	float			ovlap_;
	OD::Color		seiscolor_;
	LogStyle		style_;
    };

    const OD::LineStyle&	lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);

    void			initializeData(const LogParams&,int);
    float			getValue(const TypeSet<Coord3Value>&,int,bool,
					 const LinScaler&) const;
    Coord3			getPos(const TypeSet<Coord3Value>&,int) const;
    void			setLogColor(const OD::Color&,Side);
    const OD::Color&		logColor(Side) const;
    const OD::Color&		logFillColor(int) const;
    void			clearLog(Side);

    void			setLogLineDisplayed(bool,Side);
    bool			logLineDisplayed(Side) const;
    void			setLogWidth(float,Side);
    float			getLogWidth(Side) const;
    void			setLogLineWidth(int,Side);
    int				getLogLineWidth() const;
    void			showLogs(bool);
    void			showLog(bool,Side);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;
    void			setLogStyle(int,Side);
    void			setLogFill(bool,Side);
    void			setLogBlock(bool,int);
    void			setOverlapp(float,Side);
    void			setRepeat( int,Side );
    void			removeLogs();
    void			setTrackProperties(OD::Color&,int);
    void			setLogFillColorTab(const LogParams&,Side);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;
    void			setPixelDensity(float) override;
    float			getPixelDensity() const override
				{ return pixeldensity_;}
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    void			setLogData(const TypeSet<Coord3Value>& crdvals,
					 const TypeSet<Coord3Value>& crdvalsF,
					const LogParams& lp, bool isFilled );
    void			setLogTubeDisplay(Side side,bool yn);

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar& par) override;
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

    /// for pdf3d
    const visBase::MarkerSet*	getMarkerSet() const { return markerset_; }
    bool			hasLog(Side side) const;
    BufferString		getLogName(Side side) const;
    bool			getLogOsgData(LogStyle style,Side side,
					      TypeSet<Coord3>&coords,
					      TypeSet<OD::Color>& colors,
					      TypeSet<TypeSet<int> >& pss,
					      TypeSet<Coord3>& normals,
					      bool path) const;
    void			getLogStyle(Side,int&) const;
    unsigned int		getRepeat(Side side) const;
    float			getRepeatStep(Side side) const;
    const Text2*		getWellTopText() const { return welltoptxt_; }
    const Text2*		getWellBottomText() const{ return wellbottxt_; }


protected:

    void			transformZIfNeeded(Coord3&) const;

				~Well();
    PolyLine*			track_;
    MarkerSet*			markerset_;
    osgGeo::WellLog*		leftlogdisplay_;
    osgGeo::WellLog*		rightlogdisplay_;
    osgGeo::WellLog*		centerlogdisplay_;

    Text2*			welltoptxt_;
    Text2*			wellbottxt_;
    Text2*			markernames_;
    const mVisTrans*		transformation_;

    bool			showmarkers_;
    bool			showlogs_;

    float			pixeldensity_;
    ZAxisTransform*		zaxistransform_;
    int				voiidx_;
    bool			displaytube_[3];
    bool			displaylog_[3];
    BufferStringSet		lognames_;

private:

    void			updateText(Text* vistxt,const char* txt,
					   const Coord3* pos,
					   const FontData& fnt,
					   bool sizedynamic = true);

    void			getLinScale(const LogParams&,
					    LinScaler&,
					    bool isFill = true);
    void			getLinScaleRange( const LinScaler&,
				    Interval<float>&, float&, float&, bool);
    void			updateMakerSize(float sizefactor);
    void			updateMakerNamePosition(Side side,
							float sizefactor);
    osgGeo::WellLog*&		getLogDisplay(Side);
    const osgGeo::WellLog*	getLogDisplay(Side) const;


};

} // namespace visBase

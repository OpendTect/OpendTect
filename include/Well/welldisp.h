#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "wellcommon.h"
#include "namedmonitorable.h"
#include "color.h"
#include "coltab.h"
#include "ranges.h"
#include "fontdata.h"
#include "bufstringset.h"


namespace Well
{

mExpClass(Well) BasicDispProps : public Monitorable
{
public:

    typedef int	    SizeType;

		    BasicDispProps(SizeType);
		    ~BasicDispProps();
		    mDeclAbstractMonitorableAssignment(BasicDispProps);

    mImplSimpleMonitoredGetSet(inline,color,setColor,Color,color_,
				cColorChg());
    mImplSimpleMonitoredGetSet(inline,size,setSize,SizeType,size_,
				cSizeChg());
    mImplSimpleMonitoredGetSet(inline,font,setFont,FontData,font_,
				cFontChg());

    static SizeType	cDefaultFontSize()	{ return 10; }

    static ChangeType	cColorChg()		{ return 2; }
    static ChangeType	cSizeChg()		{ return 3; }
    static ChangeType	cFontChg()		{ return 4; }

    virtual const char*	subjectName() const	= 0;

protected:

    Color	color_;
    SizeType	size_;
    FontData	font_;

    void		baseUsePar(const IOPar&,const char*,const char*);
    void		baseFillPar(IOPar&,const char*) const;

};


mExpClass(Well) TrackDispProps : public BasicDispProps
{
public:
		    TrackDispProps();
		    ~TrackDispProps();
		    mDeclMonitorableAssignment(TrackDispProps);

    mImplSimpleMonitoredGetSet(inline,dispAbove,setDispAbove,bool,
				dispabove_,cDispPosChg());
    mImplSimpleMonitoredGetSet(inline,dispBelow,setDispBelow,bool,
				dispbelow_,cDispPosChg());
    mImplSimpleMonitoredGetSet(inline,nameSizeDynamic,setNameSizeDynamic,
				bool,nmsizedynamic_,cSizeChg());

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    static ChangeType   cDispPosChg()		{ return 5; }
    virtual const char* subjectName() const	{ return "Track"; }

protected:

    bool		dispabove_;
    bool		dispbelow_;
    bool		nmsizedynamic_;

};

mExpClass(Well) MarkerDispProps : public BasicDispProps
{
public:

    typedef int		ShapeType;
    typedef int		HeightType;

			MarkerDispProps();
			~MarkerDispProps();
			mDeclMonitorableAssignment(MarkerDispProps);

    mImplSimpleMonitoredGetSet(inline,shapeType,setShapeType,ShapeType,
				shapetype_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,cylinderHeight,setCylinderHeight,
				HeightType,cylinderheight_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,singleColor,setSingleColor,bool,
				issinglecol_,cColorChg());
    mImplSimpleMonitoredGetSet(inline,nameSizeDynamic,setNameSizeDynamic,bool,
				nmsizedynamic_,cSizeChg());
    mImplSimpleMonitoredGetSet(inline,nameColor,setNameColor,Color,
				nmcol_,cColorChg());
    mImplSimpleMonitoredGetSet(inline,sameNameCol,setSameNameCol,bool,
				samenmcol_,cColorChg());
    mImplSimpleMonitoredGetSet(inline,selMarkerNames,setSelMarkerNames,
			    BufferStringSet,selmarkernms_,cMarkerNmsChg());

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    void		addSelMarkerName(const char*);
    void		removeSelMarkerName(const char*);

    static ChangeType   cShapeChg()		{ return 5; }
    static ChangeType   cMarkerNmsChg()	{ return 6; }
    virtual const char* subjectName() const	{ return "Markers"; }

protected:

    ShapeType		shapetype_;
    HeightType		cylinderheight_;
    bool		issinglecol_;
    Color		nmcol_;
    bool		samenmcol_;
    BufferStringSet	selmarkernms_;
    bool		nmsizedynamic_;

};

mExpClass(Well) LogDispProps : public BasicDispProps
{
public:

    typedef int	    WidthType;
    typedef int	    StyleType;

		    LogDispProps();
		    ~LogDispProps();
		    mDeclMonitorableAssignment(LogDispProps);

    virtual const char* subjectName() const	{ return "Log"; }

    static ChangeType   cNameChg()		{ return 1; }
    static ChangeType   cShapeChg()		{ return 5; }
    static ChangeType   cScaleChg()		{ return 6; }

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    mImplSimpleMonitoredGetSet(inline,logName,setLogName,BufferString,
				logname_,cNameChg());
    mImplSimpleMonitoredGetSet(inline,fillName,setFillName,BufferString,
				fillname_,cNameChg());
    mImplSimpleMonitoredGetSet(inline,seqName,setSeqName,BufferString,
				seqname_,cNameChg());

    mImplSimpleMonitoredGetSet(inline,clipRate,setClipRate,float,
				cliprate_,cScaleChg());
    mImplSimpleMonitoredGetSet(inline,range,setRange,Interval<float>,
				range_,cScaleChg());
    mImplSimpleMonitoredGetSet(inline,fillRange,setFillRange,
			       Interval<float>,fillrange_,cScaleChg());
    mImplSimpleMonitoredGetSet(inline,isLogarithmic,setIsLogarithmic,bool,
			       islogarithmic_,cScaleChg());
    mImplSimpleMonitoredGetSet(inline,isDataRange,setIsDataRange,bool,
			       isdatarange_,cScaleChg());

    mImplSimpleMonitoredGetSet(inline,fillLeft,setFillLeft,bool,
			       isleftfill_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,fillRight,setFillRight,bool,
			       isrightfill_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,revertLog,setRevertLog,bool,
			       islogreverted_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,repeat,setRepeat,int,repeat_,
				cShapeChg());
    mImplSimpleMonitoredGetSet(inline,repeatOverlap,setRepeatOverlap,float,
				repeatovlap_,cShapeChg());
    mImplSimpleMonitoredGetSet(inline,style,setStyle,StyleType,style_,
				cShapeChg());
    mImplSimpleMonitoredGetSet(inline,logWidth,setLogWidth,WidthType,
				logwidth_,cShapeChg());

    mImplSimpleMonitoredGetSet(inline,singleColor,setSingleColor,bool,
			       issinglecol_,cColorChg());
    mImplSimpleMonitoredGetSet(inline,seqUseMode,setSeqUseMode,
				ColTab::SeqUseMode,sequsemode_,cColorChg());
    mImplSimpleMonitoredGetSet(inline,seisColor,setSeisColor,Color,
			       seiscolor_,cColorChg());

protected:

    BufferString	logname_;
    BufferString	fillname_;
    BufferString	seqname_;

    float		cliprate_;
    Interval<float>	range_;
    Interval<float>	fillrange_;
    bool		islogarithmic_;
    bool		isdatarange_;

    bool		isleftfill_;
    bool		isrightfill_;
    bool		islogreverted_;
    int			repeat_;
    float		repeatovlap_;
    StyleType		style_;
    WidthType		logwidth_;

    bool		issinglecol_;
    ColTab::SeqUseMode	sequsemode_;
    Color		seiscolor_;

};

inline const char* sKey2DDispProp()	{ return "2D Display"; }
inline const char* sKey3DDispProp()	{ return "3D Display"; }

/*!\brief Display properties of a well. */

mExpClass(Well) DisplayProperties : public NamedMonitorable
{
public:

    typedef BasicDispProps::SizeType	SizeType;
    typedef int				LogPairID; // yeah well. hit me.

			DisplayProperties(const char* nm=sKey3DDispProp());
			~DisplayProperties();
			mDeclMonitorableAssignment(DisplayProperties);
			mDeclInstanceCreatedNotifierAccess(DisplayProperties);

    TrackDispProps&	track()			{ return track_; }
    const TrackDispProps& track() const		{ return track_; }
    MarkerDispProps&	markers()		{ return markers_; }
    const MarkerDispProps& markers() const	{ return markers_; }


    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;

    static ChangeType	cLogPairAdded()		{ return 2; }
    static ChangeType	cLogPairRemove()	{ return 3; }
    static ChangeType	cDispStratChg()		{ return 4; }

    mImplSimpleMonitoredGetSet(inline,displayStrat,setDisplayStrat,bool,
				displaystrat_,cDispStratChg());

    static DisplayProperties&	defaults();
    static void		commitDefaults();
    bool		valsAreDefaults() const	{ return isdefaults_; }

    virtual const char* subjectName() const	{ return name().buf(); }

protected:

    TrackDispProps	track_;
    MarkerDispProps	markers_;
    bool		isdefaults_;
    bool		displaystrat_; //2d only

    void		subobjChgCB(CallBacker*);

private:

    void		init();

};


mExpClass(Well) DisplayProperties3D : public DisplayProperties
{
public:
			DisplayProperties3D();
			~DisplayProperties3D();
			mDeclMonitorableAssignment(DisplayProperties3D);

    enum Position	{ Left=0, Right, Tube };
    static ChangeType	cLogAdd()	{ return 31; }
    static ChangeType	cLogRemove()	{ return 32; }
    static ChangeType	cLogChange()	{ return 33; }

    void		addLog(const Position);
    void		removeLog(Position);
    LogDispProps*	leftLog() const
			{ return leftlog_; }
    LogDispProps*	rightLog() const
			{ return rightlog_; }
    LogDispProps*	logTube() const
			{ return logtube_; }

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

protected:
    LogDispProps*	leftlog_ = nullptr;
    LogDispProps*	rightlog_ = nullptr;
    LogDispProps*	logtube_ = nullptr;

};

mExpClass(Well) DisplayProperties2D : public DisplayProperties
{
public:

    mExpClass(Well) LogPanelProps : public NamedMonitorable
    {
	public:
				LogPanelProps(const char* nm="Panel");
				LogPanelProps(const LogPanelProps&);

	    static int		maximumNrOfLogs()	{ return 4; }
	    static ChangeType	cLogAddToPanel()	{ return 20; }
	    static ChangeType	cLogRemoveFromPanel()	{ return 21; }
	    static ChangeType	cLogNameChg()		{ return 22; }
	    static ChangeType	cLogScaleChg()		{ return 23; }

	    bool		addLog();
	    void		removeLog(int);
	    LogDispProps*	getLog(int id);
	    const LogDispProps* getLog(int id) const;
	    void		logChangeCB(CallBacker*);
	    void		fillPar(IOPar&) const;
	    void		usePar(const IOPar&);

	    ManagedObjectSet<LogDispProps> logs_;
    };

			DisplayProperties2D();
			mDeclMonitorableAssignment(DisplayProperties2D);

    static int		maximumNrOfLogPanels()	{ return 6; }
    static ChangeType	cPanelAdded()	{ return 23; }
    static ChangeType	cPanelRemove()	{ return 24; }

    void			addLogPanel();
    void			removeLogPanel(int panelid);
    int				nrPanels() const;
    const LogPanelProps*	getLogPanel(int panelid) const;
    LogPanelProps*		getLogPanel(int panelid);
    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:
    ManagedObjectSet<LogPanelProps> logpanels_;
};

} // namespace

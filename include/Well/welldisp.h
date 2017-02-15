#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "wellcommon.h"
#include "namedobj.h"
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

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    static ChangeType   cDispPosChg()		{ return 5; }
    virtual const char* subjectName() const	{ return "Track"; }

protected:

    bool		dispabove_;
    bool		dispbelow_;

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

    virtual void	usePar(const IOPar&,bool isleft);
    virtual void	fillPar(IOPar&,bool isleft) const;

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

    int			nrLogPairs() const;
    LogDispProps&	log(bool first_is_left,LogPairID nr=0);
    const LogDispProps&	log(bool first_is_left,LogPairID nr=0) const;
    LogPairID		addLogPair();
    void		setNrLogPairs(int);
    bool		removeLogPair(LogPairID); // refuses to remove last one

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

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
    ManagedObjectSet<LogDispProps> logs_;
    bool		displaystrat_; //2d only
    bool		isdefaults_;

    LogPairID		doAddLogPair();
    void		copyLogPairsFrom(const DisplayProperties&);
    void		addCBsToLogPair(LogPairID);
    void		subobjChgCB(CallBacker*);
    int			idx4PairID( LogPairID id, bool scnd=false ) const
			{ return scnd ? 2*id + 1 : 2*id; }
    int			pairID4Idx( int idx ) const
			{ return idx / 2; }
    int			nrPairs() const
			{ return logs_.size() / 2; }
    bool		isIDAvailable( LogPairID id )
			{ return logs_.size() > id * 2; }

private:

    void		init();

};

} // namespace

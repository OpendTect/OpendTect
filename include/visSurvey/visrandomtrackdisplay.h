#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2003
________________________________________________________________________

-*/

#include "vissurveycommon.h"
#include "vismultiattribsurvobj.h"
#include "seisdatapack.h"
#include "probe.h"

namespace visBase
{
    class PolyLine;
    class RandomTrackDragger;
    class TexturePanelStrip;
}

class RandomSeisDataPack;

namespace Geometry
{
    class RandomLine;
}

namespace visSurvey
{

/*!\brief Used for displaying a random or arbitrary line.

    RandomTrackDisplay is the front-end class for displaying arbitrary lines.
    The complete line consists of separate sections connected at
    inline/crossline positions, called knots or nodes. Several functions are
    available for adding or inserting node positions. The depth range of the
    line can be changed by <code>setDepthInterval(const Interval<float>&)</code>
*/

mExpClass(visSurvey) RandomTrackDisplay : public MultiTextureSurveyObject

{ mODTextTranslationClass(RandomTrackDisplay);
public:
				RandomTrackDisplay();
				mDefaultFactoryInstantiation(
				    SurveyObject,RandomTrackDisplay,
				    "RandomTrackDisplay",
				    toUiString(sFactoryKeyword()));
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    void			setProbe(Probe*);
    int				getRandomLineID() const;
    Geometry::RandomLine*	getRandomLine();

    bool			isInlCrl() const { return true; }
    bool			isSection() const { return true; }

    int				nrResolutions() const	{ return 3; }
    void			setResolution(int,TaskRunner*);

    bool			hasPosModeManipulator() const	{ return true; }
    void			showManipulator(bool yn);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const { return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;

    bool			canDuplicate() const		{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const;
    DBKey			getDBKey() const;

    bool			allowMaterialEdit() const { return true; }

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;

    TypeSet<BinID>*		getPath()		{ return &trcspath_; }
    TrcKeyPath*			getTrcKeyPath()		{ return &tkpath_; }
				//!<BinID-based coding: inner nodes single
    void			getDataTraceBids(TypeSet<BinID>&) const;
				//!<Segment-based coding: inner nodes doubled

    void			getTraceKeyPath(TrcKeyPath&,
                                                TypeSet<Coord>*) const;
    Interval<float>		getDataTraceRange() const;
    TypeSet<Coord>		getTrueCoords() const;

    bool			setDataPackID(int attrib,DataPack::ID,
						TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    bool			canAddNode(int nodenr) const;
				/*!< If nodenr<nrNodes the function Checks if
				     a node can be added before the nodenr.
				     If nodenr==nrNodes, it checks if a node
				     can be added. */
    void			addNode(int nodenr);
				/*!< If nodenr<nrNodes, a node is added before
				     the nodenr. If nodenr==nrNodes, a node is
				     added at the end. */

    int				nrNodes() const;
    void			addNode(const BinID&);
    void			insertNode(int,const BinID&);
    void			setNodePos(int,const BinID&);
    BinID			getNodePos(int) const;
    BinID			getManipNodePos(int) const;
    void			getAllNodePos(TypeSet<BinID>&) const;
    TypeSet<BinID>*		getNodes()		{ return &nodes_; }
    void			removeNode(int);
    void			removeAllNodes();
    bool			setNodePositions(const TypeSet<BinID>&);
    void			lockGeometry(bool);
    bool			isGeometryLocked() const;

    TrcKeyZSampling		getTrcKeyZSampling(int attrib) const;
    void			setDepthInterval(const Interval<float>&);
    Interval<float>		getDepthInterval() const;

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    bool			getSelMousePosInfo(const visBase::EventInfo&,
						   Coord3&, BufferString&,
						   BufferString&) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&, BufferString&,
						BufferString&) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;

    int				getSelNodeIdx() const	{ return selnodeidx_; }
				//!<knotidx>=0, panelidx<0

    NotifierAccess*		posChanged()		{ return &poschanged_; }
    virtual NotifierAccess*	getMovementNotifier()	{ return &moving_; }
    NotifierAccess*		getManipulationNotifier() {return &nodemoving_;}

    int				getClosestPanelIdx(const Coord&,
						   double* fracptr=0) const;
    Coord3			getNormal(const Coord3&) const;
    virtual float		calcDist(const Coord3&) const;
    virtual bool		allowsPicks() const		{ return true; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    bool			canBDispOn2DViewer() const	{ return true; }
    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			setRightHandSystem(bool);

    visBase::TexturePanelStrip* getTexturePanelStrip() const
				{ return panelstrip_; }
    BufferString		getRandomLineName() const;

    Notifier<RandomTrackDisplay> moving_;
    Notifier<RandomTrackDisplay> nodemoving_;
    Notifier<RandomTrackDisplay> poschanged_;

    const uiString&		errMsg() const { return errmsg_; }
    void			setPolyLineMode(bool yn);
    bool			createFromPolyLine();
    void			setColor(Color);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    virtual void		annotateNextUpdateStage(bool yn);
    virtual void		setPixelDensity(float);

    static const char*		sKeyPanelDepthKey()  { return "PanelDepthKey"; }
    static const char*		sKeyPanelPlaneKey()  { return "PanelPlaneKey"; }
    static const char*		sKeyPanelRotateKey() { return "PanelRotateKey";}

protected:
				~RandomTrackDisplay();

    void			addNodeInternal(const BinID&);
    void			insertNodeInternal(int,const BinID&);
    void			removeNodeInternal(int);
    void			movingNodeInternal(int selnodeidx);
    void			finishNodeMoveInternal();

    void			setNodePositions(const TypeSet<BinID>&,
						 bool onlyinternal);

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;

    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    void			getDataTraceBids(TypeSet<BinID>&,
						 TypeSet<int>* segments) const;
    BinID			proposeNewPos(int node) const;
    bool			isMappingTraceOfBid(BinID bid,int trcidx,
						    bool forward) const;
    void			updateTexOriginAndScale(
					    int attrib,const TrcKeyPath&,
					    const StepInterval<float>& zrg);
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =0);
    void			setNodePos(int,const BinID&,bool check);

    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			geomChangeCB(CallBacker*);
    void			geomNodeMoveCB( CallBacker*);

    void			nodeMoved(CallBacker*);
    void			draggerRightClick(CallBacker*);

    void			mouseCB(CallBacker*);
    void			pickCB(CallBacker*);
    bool			isPicking() const;

    bool			checkValidPick(const visBase::EventInfo&) const;
    void			addPickPos(const Coord3& pos);
    void			removePickPos(const Coord3&);
    void			removePickPos(int polyidx);
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool,bool);

    void			updatePanelStripPath();
    void			setPanelStripZRange(const Interval<float>&);
    float			appliedZRangeStep() const;
    void			draggerMoveFinished(CallBacker*);
    void			updateMouseCursorCB(CallBacker*);

    int				nrgeomchangecbs_;
    TypeSet<int>*		premovingselids_;
    bool			geomnodejustmoved_;

    RefMan<Probe>		probe_;
    Geometry::RandomLine*	rl_;
    visBase::TexturePanelStrip*	panelstrip_;

    visBase::RandomTrackDragger* dragger_;

    visBase::PolyLine*		polyline_;
    visBase::MarkerSet*		markerset_;

    visBase::EventCatcher*	eventcatcher_;
    MouseCursor&		mousecursor_;

    int					selnodeidx_;
    RefObjectSet<RandomSeisDataPack>	datapacks_;
    RefObjectSet<RandomSeisDataPack>	transfdatapacks_;

    TypeSet<BinID>		trcspath_;
    TrcKeyPath			tkpath_;
				//TODO replace trcspath_ by tkpath_;
    TypeSet<BinID>		nodes_;

    ZAxisTransform*		datatransform_;
    Interval<float>		depthrg_;
    int				voiidx_;

    struct UpdateStageInfo
    {
	float			oldzrgstart_;
    };
    UpdateStageInfo		updatestageinfo_;

    bool			lockgeometry_;
    bool			ismanip_;

    bool			ispicking_;
    bool			polylinemode_;
    int				pickstartnodeidx_;

    bool			interactivetexturedisplay_;
    int				originalresolution_;

    static const char*		sKeyTrack();
    static const char*		sKeyNrKnots();
    static const char*		sKeyKnotPrefix();
    static const char*		sKeyDepthInterval();
    static const char*		sKeyLockGeometry();
};

} // namespace visSurvey

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2003
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "mousecursor.h"
#include "ranges.h"
#include "seisdatapack.h"
#include "vismultiattribsurvobj.h"

class TrcKeyZSampling;

namespace visBase
{
    class EventCatcher;
    class PolyLine;
    class RandomTrackDragger;
    class MarkerSet;
    class TexturePanelStrip;
}

namespace Geometry
{
    class RandomLine;
}


namespace visSurvey
{

class Scene;

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
				    visSurvey::SurveyObject,RandomTrackDisplay,
				    "RandomTrackDisplay",
				    toUiString(sFactoryKeyword()));

    void			setRandomLineID(RandomLineID id);
    RandomLineID		getRandomLineID() const;
    Geometry::RandomLine*	getRandomLine();

    int				nameNr() const { return namenr_; }
				/*!<\returns a number that is unique for
				     this rtd, and is present in its name. */

    bool			isInlCrl() const { return true; }

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
    MultiID			getMultiID() const;

    bool			allowMaterialEdit() const { return true; }

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const;

    TypeSet<BinID>*		getPath()		{ return &trcspath_; }
				//!<BinID-based coding: inner nodes single
    void			getDataTraceBids(TypeSet<BinID>&) const;
    void			getDataTraceBids(TrcKeyPath&) const;
				//!<Segment-based coding: inner nodes doubled

    void			getTraceKeyPath(TrcKeyPath&,
						TypeSet<Coord>*) const;
    Interval<float>		getDataTraceRange() const;
    TypeSet<Coord>		getTrueCoords() const;

    bool			setDataPackID(int attrib,DataPack::ID,
						TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::MgrID	getDataPackMgrID() const
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
    void			getAllNodePos(TrcKeyPath&) const;
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

    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&, BufferString&,
						BufferString&) const;

    int				getSelNodeIdx() const	{ return selnodeidx_; }
				//!<knotidx>=0, panelidx<0

    virtual NotifierAccess*	getMovementNotifier()	{ return &moving_; }
    NotifierAccess*		getManipulationNotifier() {return &nodemoving_;}

    int				getClosestPanelIdx(const Coord&) const;
    Coord3			getNormal(const Coord3&) const;
    virtual float		calcDist(const Coord3&) const;
    virtual bool		allowsPicks() const		{ return true; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    bool			canBDispOn2DViewer() const	{ return true; }
    void			setSceneEventCatcher(visBase::EventCatcher*);
    visBase::TexturePanelStrip* getTexturePanelStrip() const
				{ return panelstrip_; }
    BufferString		getRandomLineName() const;


    Notifier<RandomTrackDisplay> moving_;
    Notifier<RandomTrackDisplay> nodemoving_;

    const char*			errMsg() const { return errmsg_.str(); }
    void			setPolyLineMode(bool yn);
    bool			createFromPolyLine();
    void			setColor(OD::Color);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    virtual void		annotateNextUpdateStage(bool yn);
    virtual void		setPixelDensity(float);
    const TrcKeyPath*		getTrcKeyPath()
				{ return &trckeypath_; }

    static const char*		sKeyPanelDepthKey()  { return "PanelDepthKey"; }
    static const char*		sKeyPanelPlaneKey()  { return "PanelPlaneKey"; }
    static const char*		sKeyPanelRotateKey() { return "PanelRotateKey";}

protected:
				~RandomTrackDisplay();

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;

    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    void			getDataTraceBids(TrcKeyPath&,
						 TypeSet<int>* segments) const;
    BinID			proposeNewPos(int node) const;
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =0);
    void			setNodePos(int,const BinID&,bool check);

    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			geomChangeCB(CallBacker*);
    void			nodeMoved(CallBacker*);
    void			draggerRightClick(CallBacker*);

    void			pickCB(CallBacker*);
    bool			checkValidPick(const visBase::EventInfo&,
					       const Coord3& pos) const;
    void			setPickPos(const Coord3& pos);
    void			removePickPos(const Coord3&);
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool,bool);

    void			updatePanelStripPath();
    void			setPanelStripZRange(const Interval<float>&);
    float			appliedZRangeStep() const;
    void			draggerMoveFinished(CallBacker*);
    void			updateMouseCursorCB(CallBacker*);

    int				nrgeomchangecbs_ = 0;
    TypeSet<int>*		premovingselids_ = nullptr;
    bool			geomnodejustmoved_ = false;

    Geometry::RandomLine*	rl_ = nullptr;
    visBase::TexturePanelStrip*	panelstrip_;

    visBase::RandomTrackDragger* dragger_;

    visBase::PolyLine*		polyline_;
    visBase::MarkerSet*		markerset_;

    visBase::EventCatcher*	eventcatcher_ = nullptr;
    MouseCursor			mousecursor_;

    int				selnodeidx_;
    RefObjectSet<RandomSeisDataPack>	datapacks_;
    RefObjectSet<RandomSeisDataPack>	transfdatapacks_;

    TypeSet<BinID>		trcspath_;
    TypeSet<BinID>		nodes_;

    ZAxisTransform*		datatransform_ = nullptr;
    Interval<float>		depthrg_;
    int				voiidx_ = -1;

    TrcKeyPath			trckeypath_;
    int				pickstartnodeidx_ = -1;
    bool			ispicking_ = false;
    int				oldstyledoubleclicked_ = 0;

    struct UpdateStageInfo
    {
	float			oldzrgstart_;
    };
    UpdateStageInfo		updatestageinfo_;

    bool			lockgeometry_ = false;
    bool			ismanip_ = false;
    int				namenr_;
    bool			polylinemode_ = false;
    bool			interactivetexturedisplay_ = false;
    int				originalresolution_ = -1;

    static const char*		sKeyTrack();
    static const char*		sKeyNrKnots();
    static const char*		sKeyKnotPrefix();
    static const char*		sKeyDepthInterval();
    static const char*		sKeyLockGeometry();

    void			updateTexOriginAndScale(
					    int attrib,const TrcKeyPath&,
					    const StepInterval<float>& zrg);
public:

    bool			getSelMousePosInfo(const visBase::EventInfo&,
						   Coord3&, BufferString&,
						   BufferString&) const;
    const TypeSet<BinID>*	getPath() const		{ return &trcspath_; }
    mDeprecated("Use TrcKey")
    void			getAllNodePos(TypeSet<BinID>&) const;

protected:
    int				getClosestPanelIdx(const Coord&,
						   double* fracptr) const;
    void			mouseCB(CallBacker*);
    bool			isPicking() const;
    void			removePickPos(int polyidx);

    void			addNodeInternal(const BinID&);
    void			insertNodeInternal(int,const BinID&);
    void			removeNodeInternal(int);
    void			movingNodeInternal(int selnodeidx);
    void			finishNodeMoveInternal();
    void			geomNodeMoveCB( CallBacker*);
    void			setNodePositions(const TypeSet<BinID>&,
						 bool onlyinternal);

    bool			isMappingTraceOfBid(BinID bid,int trcidx,
						    bool forward=true) const;

    void			snapZRange(Interval<float>&);
};

} // namespace visSurvey


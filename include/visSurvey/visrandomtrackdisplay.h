#ifndef visrandomtrackdisplay_h
#define visrandomtrackdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2003
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "vismultiattribsurvobj.h"
#include "ranges.h"

class TrcKeyZSampling;

namespace visBase
{
    class EventCatcher;
    class PolyLine;
    class RandomTrackDragger;
    class MarkerSet;
    class TexturePanelStrip;
}


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying a random or arbitrary line.

    RandomTrackDisplay is the front-end class for displaying arbitrary lines.
    The complete line consists of separate sections connected at
    inline/crossline positions, called knots or nodes. Several functions are
    available for adding or inserting knot positions. The depth range of the
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

    int				nameNr() const { return namenr_; }
				/*!<\returns a number that is unique for
				     this rtd, and is present in its name. */

    bool			isInlCrl() const { return true; }

    int				nrResolutions() const	{ return 3; }
    void			setResolution(int,TaskRunner*);

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
				//!<BinID-based coding: inner knots single
    void			getDataTraceBids(TypeSet<BinID>&) const;
				//!<Segment-based coding: inner knots doubled

    Interval<float>		getDataTraceRange() const;
    TypeSet<Coord>		getTrueCoords() const;

    bool			setDataPackID(int attrib,DataPack::ID,
						TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    bool			canAddKnot(int knotnr) const;
				/*!< If knotnr<nrKnots the function Checks if
				     a knot can be added before the knotnr.
				     If knotnr==nrKnots, it checks if a knot
				     can be added. */
    void			addKnot(int knotnr);
				/*!< If knotnr<nrKnots, a knot is added before
				     the knotnr. If knotnr==nrKnots, a knot is
				     added at the end. */

    int				nrKnots() const;
    void			addKnot(const BinID&);
    void			insertKnot(int,const BinID&);
    void			setKnotPos(int,const BinID&);
    BinID			getKnotPos(int) const;
    BinID			getManipKnotPos(int) const;
    void			getAllKnotPos(TypeSet<BinID>&) const;
    TypeSet<BinID>*		getKnots()		{ return &knots_; }
    void			removeKnot(int);
    void			removeAllKnots();
    bool			setKnotPositions(const TypeSet<BinID>&);
    void			lockGeometry(bool);
    bool			isGeometryLocked() const;

    TrcKeyZSampling		getTrcKeyZSampling(int attrib) const;
    void			setDepthInterval(const Interval<float>&);
    Interval<float>		getDepthInterval() const;

    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&, BufferString&,
						BufferString&) const;

    int				getSelKnotIdx() const	{ return selknotidx_; }

    virtual NotifierAccess*	getMovementNotifier()	{ return &moving_; }
    NotifierAccess*		getManipulationNotifier() {return &knotmoving_;}

    Coord3			getNormal(const Coord3&) const;
    virtual float		calcDist(const Coord3&) const;
    virtual bool		allowsPicks() const		{ return true; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    bool			canBDispOn2DViewer() const	{ return true; }
    void			setSceneEventCatcher(visBase::EventCatcher*);


    Notifier<RandomTrackDisplay> moving_;
    Notifier<RandomTrackDisplay> knotmoving_;

    const char*			errMsg() const { return errmsg_.str(); }
    void			setPolyLineMode(bool mode );
    bool			createFromPolyLine();
    void			setColor(Color);

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    virtual void		annotateNextUpdateStage(bool yn);
    virtual void		setPixelDensity(float);


protected:
				~RandomTrackDisplay();

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;

    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    void			getDataTraceBids(TypeSet<BinID>&,
						 TypeSet<int>* segments) const;
    BinID			proposeNewPos(int knot) const;
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib);

    void			setKnotPos(int,const BinID&,bool check);

    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			knotMoved(CallBacker*);
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

    visBase::TexturePanelStrip*	panelstrip_;

    visBase::RandomTrackDragger* dragger_;

    visBase::PolyLine*		polyline_;
    visBase::MarkerSet*		markerset_;
    visBase::EventCatcher*	eventcatcher_;

    int				selknotidx_;
    TypeSet<DataPack::ID>	datapackids_;
    TypeSet<DataPack::ID>	transfdatapackids_;
    TypeSet<BinID>		trcspath_;
    TypeSet<BinID>		knots_;

    ZAxisTransform*		datatransform_;
    Interval<float>		depthrg_;
    int				voiidx_;

    float			oldzrgstart_;

    bool			lockgeometry_;
    bool			ismanip_;
    int				namenr_;
    bool			polylinemode_;
    bool			interactivetexturedisplay_;
    static const char*		sKeyTrack();
    static const char*		sKeyNrKnots();
    static const char*		sKeyKnotPrefix();
    static const char*		sKeyDepthInterval();
    static const char*		sKeyLockGeometry();
};

} // namespace visSurvey

#endif


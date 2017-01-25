#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id$
________________________________________________________________________

-*/


#include "vissurveymod.h"
#include "bufstringset.h"
#include "draw.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "emobject.h"

class ZAxisTransform;

namespace EM { class EMManager; class EMObject; }
namespace Geometry { class Element; }
namespace visBase
{
    class DataObjectGroup;
    class DrawStyle;
    class TextureChannel2RGBA;
    class MarkerSet;
}

namespace visSurvey
{
struct EMChangeData
{
    ObjectSet<const EM::EMObjectCallbackData>	emcallbackdata_;
    Threads::Lock				lock_;

    void    addCallBackData( const EM::EMObjectCallbackData* data )
    {
	Threads::Locker locker( lock_ );
	emcallbackdata_ += data;
    }

    const EM::EMObjectCallbackData*   getCallBackData( int idx ) const
    {
	if ( idx<emcallbackdata_.size() )
	    return emcallbackdata_[idx];
	return 0;    
    }

    void    clearData()
    {
	Threads::Locker locker( lock_ );
	deepErase( emcallbackdata_ );
    }

    int	    size() const { return emcallbackdata_.size(); }
};

class MPEEditor;
class EdgeLineSetDisplay;


mExpClass(visSurvey) EMObjectDisplay : public  visBase::VisualObjectImpl
				     , public visSurvey::SurveyObject
{
public:
    const mVisTrans*		getDisplayTransformation() const;
    void			setDisplayTransformation(const mVisTrans*);
    void			setSceneEventCatcher( visBase::EventCatcher* );
    virtual bool		setZAxisTransform(ZAxisTransform*,TaskRunner*)
				{ return false; }
    const ZAxisTransform*	getZAxisTransform() const
				{ return zaxistransform_; }

    virtual bool		setEMObject(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getObjectID() const;
    virtual bool		updateFromEM(TaskRunner*);
    virtual void		updateFromMPE();

    void			showPosAttrib( int attr, bool yn);
				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker. */
    bool			showsPosAttrib( int attr ) const;
				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */
    const char*			errMsg() const { return errmsg_.str(); }

    MultiID			getMultiID() const;
    BufferStringSet		displayedSections() const;

    virtual void		setOnlyAtSectionsDisplay(bool yn);
    virtual bool		displayedOnlyAtSections() const;

    virtual void		turnOnSelectionMode(bool);
    bool			allowMaterialEdit() const { return true; }
    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const;
    void			setColor(Color);
    Color			getColor() const;

    bool			allowsPicks() const	{ return true; }

    void			getObjectInfo(BufferString&) const;
    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
						BufferString& info) const;
    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    virtual EM::SectionID	getSectionID(int visid) const		= 0;
    EM::SectionID		getSectionID(const TypeSet<int>* path) const;

    EM::PosID			getPosAttribPosID(int attrib,
					 const TypeSet<int>& path,
					 const Coord3& clickeddisplaypos) const;

    virtual bool		canRemoveSelection() const	{ return true; }
    virtual bool		removeSelections(TaskRunner*);
    virtual void		clearSelections();
    virtual void		updateAuxData()			{}

    virtual bool		setChannels2RGBA(visBase::TextureChannel2RGBA*);
    virtual visBase::TextureChannel2RGBA* getChannels2RGBA();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    NotifierAccess*		getMovementNotifier()	{ return &hasmoved; }
    Notifier<EMObjectDisplay>	changedisplay;

    void			lock(bool yn);
    NotifierAccess*		getLockNotifier()	{ return &locknotifier;}
    virtual void		doOtherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj )	=0;
    virtual void		setPixelDensity(float dpi);
    const visBase::MarkerSet*	getSeedMarkerSet() const; 


protected:
				EMObjectDisplay();
				~EMObjectDisplay();
    virtual void		removeEMStuff();

    virtual void		removeSectionDisplay(const EM::SectionID&) = 0;
    virtual bool		addSection(const EM::SectionID&,TaskRunner*)=0;

    virtual EM::PosID		findClosestNode(const Coord3&) const;
    virtual void		emChangeCB(CallBacker*);
    virtual void		clickCB(CallBacker*);
    virtual void		updatePosAttrib(int attrib);
    void			polygonFinishedCB(CallBacker*);

    virtual void		updateSelections();
    void			handleEmChange(const EM::EMObjectCallbackData&);

    Notifier<EMObjectDisplay>	hasmoved;
    Notifier<EMObjectDisplay>	locknotifier;

    const mVisTrans*			transformation_;
    ZAxisTransform*			zaxistransform_;
    visBase::EventCatcher*		eventcatcher_;

    ObjectSet<visBase::MarkerSet>	posattribmarkers_;

    TypeSet<int>			posattribs_;
    TypeSet<int>			parposattrshown_;

    EM::EMManager&			em_;
    EM::EMObject*			emobject_;
    MultiID				parmid_;
    BufferStringSet			parsections_;

    MPEEditor*				editor_;
    visBase::TextureChannel2RGBA*	channel2rgba_;
					//set in usePar,
					//should be nil when given
					//to channels_.

    TypeSet<EM::SectionID>		addsectionids_;

    mutable Color			nontexturecol_;
    mutable bool			nontexturecolisset_;
    visBase::DrawStyle*			drawstyle_;
    bool				displayonlyatsections_;
    bool				enableedit_;
    bool				restoresessupdate_;

    bool				burstalertison_;
    bool				ctrldown_;
    ObjectSet< Selector<Coord3> >	selectors_;

    static const char*			sKeyEarthModelID();
    static const char*			sKeyResolution();
    static const char*			sKeyEdit();
    static const char*			sKeyOnlyAtSections();
    static const char*			sKeyLineStyle();
    static const char*			sKeySections();
    static const char*			sKeyPosAttrShown();


private:
    void				unSelectAll();
    void				updateLockedSeedsColor();
    const TypeSet<int>			findOverlapSelectors(
						    visBase::PolygonSelection*);
public:
    virtual bool		getOnlyAtSectionsDisplay() const
				{ return displayedOnlyAtSections(); }
};

} // namespace visSurvey

#endif

#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.37 2006-03-09 17:26:20 cvskris Exp $
________________________________________________________________________


-*/


#include "bufstringset.h"
#include "draw.h"
#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


class Executor;

namespace EM { class EMManager; }
namespace Geometry { class Element; }
namespace visBase
{
    class DataObjectGroup;
    class DrawStyle;
    class IndexedPolyLine;
    class VisColorTab;
}

namespace visSurvey
{

class MPEEditor;
class EdgeLineSetDisplay;


class EMObjectDisplay :  public  visBase::VisualObjectImpl,
                         public visSurvey::SurveyObject
{
public:
    static EMObjectDisplay*	create()
				mCreateDataObj( EMObjectDisplay );

    mVisTrans*			getDisplayTransformation();
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher( visBase::EventCatcher* );

    bool			setEMObject(const EM::ObjectID&);
    EM::ObjectID		getObjectID() const { return oid; }
    bool			updateFromEM();
    void			updateFromMPE();

    void			showPosAttrib( int attr, bool yn, const Color&);
    				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker.
				*/
    bool			showsPosAttrib( int attr ) const;
    				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */

    MultiID			getMultiID() const;
    BufferStringSet		displayedSections() const;
    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;

    void			useTexture(bool yn,bool trigger=false);
    bool			usesTexture() const;

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			getOnlyAtSectionsDisplay() const;

    void			displaySurfaceData(int attrib,int auxdatanr);
    void			selectTexture(int,int);

    SurveyObject::AttribFormat	getAttributeFormat() const;
    bool			canHaveMultipleAttribs() const;
    int				nrAttribs() const;
    bool			addAttrib();
    bool			removeAttrib(int attrib);
    bool			swapAttribs(int attrib0,int attrib1);
    void			setAttribTransparency(int,unsigned char);
    unsigned char		getAttribTransparency(int) const;
    void			enableAttrib(int attrib, bool yn);
    bool			isAttribEnabled(int attrib) const;
    const Attrib::SelSpec*	getSelSpec(int) const;
    void			setSelSpec(int,const Attrib::SelSpec&);
    void			setDepthAsAttrib(int);


    bool			allowMaterialEdit() const { return true; }
    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const;
    void			setColor(Color);
    Color			getColor() const;

    void			getRandomPos(ObjectSet<BinIDValueSet>&) const;
    void			getRandomPosCache(int,
	    				ObjectSet<const BinIDValueSet>&) const;
    void			setRandomPosData( int,
					   const ObjectSet<BinIDValueSet>*);

    bool			hasStoredAttrib( int attrib ) const;

    int				nrResolutions() const;
    BufferString		getResolutionName(int) const;
    int				getResolution() const;
    void			setResolution(int);
    				/*!< 0 is automatic */

    bool			allowPicks() const	{ return true; }
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,
	    					float& val,
						BufferString& info) const;
    float			calcDist(const Coord3&) const;
    float			maxDist() const;

    int				getColTabID(int) const;

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);

    bool			usesWireframe() const;
    void			useWireframe(bool);

    void			setEdgeLineRadius(float);
    float			getEdgeLineRadius() const;

    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    EM::SectionID		getSectionID(int visid) const;
    EM::SectionID		getSectionID(const TypeSet<int>* path) const;

    EM::PosID			getPosAttribPosID( int attrib,
					   const TypeSet<int>& path ) const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    NotifierAccess*		getMovementNotification();
    Notifier<EMObjectDisplay>	hasmoved;
    Notifier<EMObjectDisplay>	changedisplay;

protected:
    					~EMObjectDisplay();
    void				removeEMStuff();

    static visBase::VisualObject*	createSection(Geometry::Element*);
    bool				addSection(EM::SectionID);
    bool				addEdgeLineDisplay(EM::SectionID);
    void				emChangeCB(CallBacker*);
    void				emEdgeLineChangeCB(CallBacker*);
    void				clickCB(CallBacker*);
    void				edgeLineRightClickCB(CallBacker*);
    void				updatePosAttrib(int attrib);
    void				updateIntersectionLines(
					   const ObjectSet<const SurveyObject>&,
					   int whichobj );
    void				otherObjectsMoved(
					   const ObjectSet<const SurveyObject>&,
					   int whichobj );

    mVisTrans*				transformation;
    mVisTrans*				translation;
    visBase::EventCatcher*		eventcatcher;
    ObjectSet<visBase::VisColorTab>	coltabs_;

    ObjectSet<visBase::VisualObject>	sections;
    TypeSet<EM::SectionID>		sectionids;

    ObjectSet<EdgeLineSetDisplay>	edgelinedisplays;

    ObjectSet<visBase::DataObjectGroup>	posattribmarkers;
    TypeSet<int>			posattribs;

    ObjectSet<visBase::IndexedPolyLine>	intersectionlines;
    TypeSet<int>			intersectionlineids;

    EM::EMManager&			em;
    EM::ObjectID			oid;
    MultiID				parmid;
    BufferStringSet			parsections;
    StepInterval<int>			parrowrg;
    StepInterval<int>			parcolrg;


    MPEEditor*				editor;

    Color				nontexturecol;
    visBase::DrawStyle*			drawstyle;
    bool				usestexture;
    bool				displayonlyatsections;
    bool				useswireframe;
    int					curtextureidx;
    float				edgelineradius;

    ObjectSet<Attrib::SelSpec>		as_;
    bool				validtexture_;

    static visBase::FactoryEntry	oldnameentry;

    static const char*			sKeyEarthModelID;
    static const char*			sKeyTexture;
    static const char*			sKeyColorTableID;
    static const char*			sKeyShift;
    static const char*			sKeyEdit;
    static const char*			sKeyWireFrame;
    static const char*			sKeyResolution;
    static const char*			sKeyOnlyAtSections;
    static const char*			sKeyLineStyle;
    static const char*			sKeyEdgeLineRadius;
    static const char*			sKeyRowRange;
    static const char*			sKeyColRange;
    static const char*			sKeySections;
};


};

#endif



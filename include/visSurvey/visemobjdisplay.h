#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.26 2005-08-18 19:38:04 cvskris Exp $
________________________________________________________________________


-*/


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

    bool			setEMObject(const MultiID&);
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

    const MultiID*		getMultiID() const { return &mid; }

    void			useTexture(bool yn);
    bool			usesTexture() const;

    void			setOnlyAtSectionsDisplay(bool yn);
    bool			getOnlyAtSectionsDisplay() const;

    void			readAuxData();
    void			selectTexture(int);
    void			selectNextTexture(bool next);

    int				getAttributeFormat() const;
    const Attrib::SelSpec*	getSelSpec() const;
    const Attrib::ColorSelSpec*	getColorSelSpec() const;
    void			setSelSpec(const Attrib::SelSpec&);
    void			setColorSelSpec(const Attrib::ColorSelSpec&);
    void			setDepthAsAttrib();

    bool			allowMaterialEdit() const { return true; }
    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const;
    void			setColor(Color);
    Color			getColor() const;

    void			fetchData( ObjectSet<BinIDValueSet>&) const;
    void			stuffData( bool forcolordata,
					   const ObjectSet<BinIDValueSet>*);

    bool			hasStoredAttrib() const;

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

    int				getColTabID() const;

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

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    NotifierAccess*		getMovementNotification();
    Notifier<EMObjectDisplay>	hasmoved;

protected:
    					~EMObjectDisplay();
    void				removeEMStuff();

    static visBase::VisualObject*	createSection(Geometry::Element*);
    void				removeAttribCache();
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
    visBase::VisColorTab*		coltab_;

    ObjectSet<visBase::VisualObject>	sections;
    TypeSet<EM::SectionID>		sectionids;

    ObjectSet<EdgeLineSetDisplay>	edgelinedisplays;

    ObjectSet<visBase::DataObjectGroup>	posattribmarkers;
    TypeSet<int>			posattribs;

    ObjectSet<visBase::IndexedPolyLine>	intersectionlines;
    TypeSet<int>			intersectionlineids;

    EM::EMManager&			em;
    MultiID				mid;
    MPEEditor*				editor;

    Color				nontexturecol;
    visBase::DrawStyle*			drawstyle;
    bool				usestexture;
    bool				displayonlyatsections;
    bool				useswireframe;
    int					curtextureidx;
    float				edgelineradius;

    Attrib::SelSpec&			as;
    Attrib::ColorSelSpec&		colas;
    ObjectSet<const float>		attribcache;
    TypeSet<int>			attribcachesz;

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
};


};

#endif



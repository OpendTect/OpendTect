#ifndef visemobjdisplay_h
#define visemobjdisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.21 2005-08-04 15:51:37 cvskris Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


class Executor;

namespace EM { class EMManager; }
namespace Geometry { class Element; }
namespace visBase { class VisColorTab; class DataObjectGroup; }

namespace visSurvey
{

class MPEEditor;


class EMObjectDisplay :  public  visBase::VisualObjectImpl,
                         public visSurvey::SurveyObject
{
public:
    static EMObjectDisplay*	create()
				mCreateDataObj( EMObjectDisplay );

    mVisTrans*			getDisplayTransformation();
    void			setDisplayTransformation(mVisTrans*);
    void			setSceneEventCatcher( visBase::EventCatcher* );

    void			removeAll();

    bool			setEMObject(const MultiID&);
    bool			updateFromEM();
    void			updateFromMPE();

    void			showPosAttrib( int attr, bool yn, const Color&);
    				/*!<Turns position attributes (as defined in
				    EM::EMObject) to be marked with a marker. */
    bool			showsPosAttrib( int attr ) const;
    				/*!<\returns wether a position attribute (as
				     defined in EM::EMObject) to be marked
				     with a marker. */

    const MultiID*		getMultiID() const { return &mid; }

    void			useTexture(bool yn);
    bool			usesTexture() const;

    void			readAuxData();
    void			selectTexture(int);
    void			selectNextTexture(bool next);

    int				getAttributeFormat() const;
    const Attrib::SelSpec*	getSelSpec() const;
    const Attrib::ColorSelSpec*	getColorSelSpec() const;
    void			setSelSpec(const Attrib::SelSpec&);
    void			setColorSelSpec(const Attrib::ColorSelSpec&);
    void			setDepthAsAttrib();

    bool			allowMaterialEdit() const	{ return true; }
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
    void			getMousePosInfo(const Coord3& pos,float& val,
						BufferString& info) const;

    int				getColTabID() const;

    Coord3			getTranslation() const;
    void			setTranslation(const Coord3&);

    bool			usesWireframe() const;
    void			useWireframe(bool);

    MPEEditor*			getEditor();
    void			enableEditing(bool yn);
    bool			isEditingEnabled() const;

    EM::SectionID		getSectionID(int visid) const;
    EM::SectionID		getSectionID(const TypeSet<int>* path) const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    					~EMObjectDisplay();
    static visBase::VisualObject*	createSection(Geometry::Element*);
    void				removeAttribCache();
    bool				addSection(EM::SectionID);
    void				emSectionChangeCB(CallBacker*);
    void				clickCB(CallBacker*);
    void				updatePosAttrib(int attrib);

    mVisTrans*				transformation;
    mVisTrans*				translation;
    visBase::EventCatcher*		eventcatcher;
    visBase::VisColorTab*		coltab_;

    ObjectSet<visBase::VisualObject>	sections;
    TypeSet<EM::SectionID>		sectionids;

    ObjectSet<visBase::DataObjectGroup>	posattribmarkers;
    TypeSet<int>			posattribs;


    EM::EMManager&			em;
    MultiID				mid;
    MPEEditor*				editor;

    Color				nontexturecol;
    bool				usestexture;
    bool				useswireframe;
    int					curtextureidx;

    Attrib::SelSpec&			as;
    Attrib::ColorSelSpec&		colas;
    ObjectSet<const float>		attribcache;
    TypeSet<int>			attribcachesz;

    static visBase::FactoryEntry	oldnameentry;
    static const char*			earthmodelidstr;
    static const char*			texturestr;
    static const char*			colortabidstr;
    static const char*			shiftstr;
    static const char*			editingstr;
    static const char*			wireframestr;
    static const char*			resolutionstr;
    static const char*			colorstr;
};


};

#endif



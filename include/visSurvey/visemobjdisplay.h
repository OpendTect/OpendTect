#ifndef vissurvemobj_h
#define vissurvemobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.12 2005-04-07 08:07:03 cvsnanne Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


class Executor;

namespace EM { class EMManager; }
namespace Geometry { class Element; }
namespace visBase { class VisColorTab; }

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
    const MultiID*		getMultiID() const { return &mid; }

    void			useTexture(bool yn);
    bool			usesTexture() const;

    void			readAuxData();
    void			selectTexture(int);
    void			selectNextTexture(bool next);

    int				getAttributeFormat() const	{ return 2; }
    bool			hasColorAttribute() const; 
    const AttribSelSpec*	getSelSpec() const;
    const ColorAttribSel*	getColorSelSpec() const;
    void			setSelSpec(const AttribSelSpec&);
    void			setColorSelSpec(const ColorAttribSel&);
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
    static visBase::VisualObject* createSection(Geometry::Element*);
    void			removeAttribCache();

    mVisTrans*			transformation;
    mVisTrans*			translation;
    visBase::EventCatcher*	eventcatcher;
    visBase::VisColorTab*	coltab_;

    ObjectSet<visBase::VisualObject> sections;
    TypeSet<EM::SectionID>	sectionids;

    EM::EMManager&		em;
    MultiID			mid;
    MPEEditor*			editor;

    Color			nontexturecol;
    bool			usestexture;
    int				curtextureidx;

    AttribSelSpec&		as;
    ColorAttribSel&		colas;
    ObjectSet<const float>	attribcache;
    TypeSet<int>		attribcachesz;

    static visBase::FactoryEntry oldnameentry;
    static const char*		earthmodelidstr;
    static const char*		texturestr;
    static const char*		colortabidstr;
    static const char*		shiftstr;
    static const char*		editingstr;
    static const char*		wireframestr;
    static const char*		resolutionstr;
    static const char*		colorstr;

};


};

#endif



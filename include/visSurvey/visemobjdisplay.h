#ifndef vissurvemobj_h
#define vissurvemobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.9 2005-03-31 15:17:34 cvsnanne Exp $
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

    static visBase::FactoryEntry oldnameentry;
    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    					~EMObjectDisplay();
    static visBase::VisualObject*	createSection(Geometry::Element*);
    void				removeAttribCache();

    mVisTrans*				transformation;
    mVisTrans*				translation;
    visBase::EventCatcher*		eventcatcher;
    visBase::VisColorTab*		coltab_;

    ObjectSet<visBase::VisualObject>	sections;
    TypeSet<EM::SectionID>		sectionids;

    EM::EMManager&			em;
    MultiID				mid;
    MPEEditor*				editor;

    Color				nontexturecol;
    bool				usestexture;

    AttribSelSpec&			as;
    ColorAttribSel&			colas;
    ObjectSet<const float>		attribcache;
    TypeSet<int>			attribcachesz;
};


};

#endif



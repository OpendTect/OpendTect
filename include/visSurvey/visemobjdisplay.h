#ifndef vissurvemobj_h
#define vissurvemobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: visemobjdisplay.h,v 1.1 2005-01-06 10:54:02 kristofer Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"


class Executor;


namespace Geometry { class Element; }



namespace visSurvey
{

class MPEEditor;


class EMObjectDisplay :  public  visBase::VisualObjectImpl,
                         public visSurvey::SurveyObject
{
public:
    static EMObjectDisplay*	create()
				mCreateDataObj( EMObjectDisplay );

    void			removeAll();

    bool			setEMObject(const MultiID&);
    bool			updateFromEM();
    const MultiID*		getMultiID() const { return &mid; }

    void			useTexture(bool yn);
    bool			usesTexture() const;
    
    bool			hasColorAttribute() const; 
    const AttribSelSpec*	getSelSpec() const;
    const ColorAttribSel*	getColorSelSpec() const;
    void			setSelSpec(const AttribSelSpec&);
    void			setColorSelSpec(const ColorAttribSel&);
    void			setDepthAsAttrib();

    MPEEditor*			getEditor();
    EM::SectionID		getSectionID(int visid) const;

protected:
    					~EMObjectDisplay();
    static visBase::VisualObject*	createSection(Geometry::Element*);
    void				removeAttribCache();

    ObjectSet<visBase::VisualObject>	sections;
    TypeSet<EM::SectionID>		sectionids;
    
    MultiID				mid;

    Color				nontexturecol;
    bool				usestexture;

    AttribSelSpec&			as;
    ColorAttribSel&			colas;
    ObjectSet<const float>		attribcache;
    TypeSet<int>			attribcachesz;
};


};

#endif



#ifndef vistext_h
#define vistext_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-22-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visosg.h"
#include "fontdata.h"
#include "visobject.h"
#include "position.h"
#include "uistring.h"

namespace osgText { class Text; class Font; }

namespace osg { class Drawable; class Vec3f; class Geode; }

namespace visBase
{

mExpClass(visBase) Text
{
public:
				Text();
				~Text();
    enum			Justification { Left, Right, Center };

    void			setPosition(const osg::Vec3f&);
    void			setPosition(const Coord3&,
					    bool scenespace = false);
    Coord3			getPosition() const;

    void			setFontData(const FontData&);
    const FontData&		getFontData() const	{ return fontdata_; }

    void			setText(const uiString&);
    const uiString&		getText() const 	{ return text_; }

    void			setColor(const Color&);
    Color			getColor() const;

    void			setJustification(Justification);

    osg::Drawable&		getDrawable();
    const osg::Drawable&	getDrawable() const;

    void			setDisplayTransformation( const mVisTrans*);

protected:
    const mVisTrans*		displaytrans_;
    osgText::Text*		osgtext_;
    uiString			text_;

    FontData			fontdata_;
};


mExpClass(visBase) OsgFontCreator
{
public:
    virtual			~OsgFontCreator() 			{}
    static osgText::Font*	create(const FontData&);
protected:
    static void			setCreator(OsgFontCreator*);
    virtual osgText::Font*	createFont(const FontData&)		= 0;
};


mExpClass(visBase) Text2 : public VisualObjectImpl
{
public:
    static Text2*		create()
				mCreateDataObj(Text2);

    int				nrTexts() const		{return texts_.size();}
    int				addText();
    void			removeText(const Text*);
    void			removeAll();

    void			setFontData(const FontData&);

    const Text*			text(int idx=0) const;
    Text*			text(int idx=0);

    void			setDisplayTransformation(const mVisTrans*);

protected:
    void			translationChangeCB(CallBacker*);
				~Text2();
    osg::Geode*			geode_;
    ManagedObjectSet<Text>	texts_;
    const mVisTrans*		displaytransform_;
};


}; // Namespace

#endif


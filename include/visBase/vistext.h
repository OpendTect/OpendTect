#ifndef vistext_h
#define vistext_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-22-2002
 RCS:		$Id: vistext.h,v 1.11 2005-02-11 11:13:25 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoFont;
class SoText2;
class SoTranslation;

namespace visBase
{

/*!\brief
is a text that always is facing the user. The size is set in printer points
on the screen. It is advisable to turn off the text when doing a viewAll,
since their sizes will corrupt the bounding box calculation.
*/

class Text : public VisualObjectImpl
{
public:
    enum			Justification { Left, Right, Center };

    virtual Coord3		position() const;
    virtual void		setPosition(const Coord3&);

    virtual float		size() const;
    virtual void		setSize(float);

    virtual const char*		getText() const			=0;
    virtual void		setText(const char*)		=0;

    virtual Justification	justification() const		=0;
    virtual void		setJustification(Justification)	=0;

    virtual void		setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    				Text();
				~Text();

    SoTranslation*		textpos;
    SoFont*			font;
    Transformation*		transformation;

    static const char*		stringstr;
    static const char*		fontsizestr;
    static const char*		justificationstr;
    static const char*		positionstr;
};


class Text2 : public Text
{
public:
    static Text2*		create()
    				mCreateDataObj(Text2);

    void			setText(const char*);
    const char*			getText() const;

    void			setJustification(Justification);
    Justification		justification() const;

protected:
    SoText2*			text;

};

}; // Namespace

#endif

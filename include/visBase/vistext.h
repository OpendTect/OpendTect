#ifndef vistext_h
#define vistext_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-22-2002
 RCS:		$Id: vistext.h,v 1.18 2011/12/16 15:57:20 cvskris Exp $
________________________________________________________________________


-*/

#include "fontdata.h"
#include "visobject.h"
#include "position.h"

class SoFont;
class SoText2;
class SoAsciiText;
class SoTranslation;

namespace visBase
{
class PickStyle;

mClass Text : public VisualObjectImpl
{
public:
    enum			Justification { Left, Right, Center };

    Coord3			position() const;
    void			setPosition(const Coord3&);

    void			setFontData(const FontData&);
    const FontData&		getFontData() const	{ return fontdata_; }

    virtual const char*		getText() const			=0;
    virtual void		setText(const char*)		=0;

    virtual Justification	justification() const		=0;
    virtual void		setJustification(Justification)	=0;

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    				Text();
				~Text();

    FontData			fontdata_;
    SoTranslation*		textpos_;
    SoFont*			font_;
    const mVisTrans*		transformation_;
    PickStyle*			pickstyle_;

    static const char*		sKeyString();
    static const char*		sKeyFontData();
    static const char*		sKeyJustification();
    static const char*		sKeyPosition();
};


/*!\brief
is a text that always is facing the user. The size is set in printer points
on the screen. It is advisable to turn off the text when doing a viewAll,
since their sizes will corrupt the bounding box calculation.
*/

mClass Text2 : public Text
{
public:
    static Text2*		create()
    				mCreateDataObj(Text2);
    				~Text2();

    void			setText(const char*);
    const char*			getText() const;

    void			setJustification(Justification);
    Justification		justification() const;

protected:
    SoText2*			text_;

};


/*!Text that is not rotated to face text. */

mClass TextBox : public Text
{
public:
    static TextBox*		create()
    				mCreateDataObj(TextBox);
    				~TextBox();

    void			setText(const char*);
    const char*			getText() const;

    void			setJustification(Justification);
    Justification		justification() const;

protected:
    SoAsciiText*		text_;

};

}; // Namespace

#endif

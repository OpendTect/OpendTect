#ifndef vistext_h
#define vistext_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-22-2002
 RCS:		$Id: vistext.h,v 1.7 2002-11-15 08:14:32 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "bufstring.h"
#include "position.h"

class SoText2;
class SoFont;
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
    enum		Justification { Left, Right, Center };
    static Text*	create()
			mCreateDataObj(Text);

    Coord3		position() const;
    void		setPosition( const Coord3& );

    BufferString	getText() const;
    void		setText(const char*);

    float		size() const;
    void		setSize(float);

    Justification	justification() const;
    void		setJustification( Justification );

    void		fillPar( IOPar&, TypeSet<int>& ) const;
    int			usePar( const IOPar& );

protected:
			~Text();
    SoTranslation*	textpos;
    SoFont*		font;
    SoText2*		text;

    static const char*	stringstr;
    static const char*	fontsizestr;
    static const char*	justificationstr;
    static const char*	positionstr;
};

}; // Namespace

#endif

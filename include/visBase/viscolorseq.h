#ifndef viscoloseq_h
#define viscoloseq_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolorseq.h,v 1.2 2002-03-11 14:38:02 kristofer Exp $
________________________________________________________________________


-*/

#include "callback.h"
#include "color.h"
#include "visdata.h"

class ColorTable;

namespace visBase
{

/*!\brief
ColorSequence describes a basic sequence of colors on a scale ranging from zero
to one.
*/

class ColorSequence : public DataObject
{
public:
    static ColorSequence*	create()
				mCreateDataObj0arg(ColorSequence);

    void			loadFromStorage( const char* nm );
			
    ColorTable&		colors();
    const ColorTable&	colors() const;
    void		colorsChanged();
			/*!< If you change the Colortable, notify me with
			     this function
			*/

    Notifier<ColorSequence>	change;
protected:
			ColorSequence();
    virtual		~ColorSequence();


    ColorTable&			coltab;
};

}; // Namespace


#endif

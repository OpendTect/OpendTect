#ifndef viscoloseq_h
#define viscoloseq_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscolorseq.h,v 1.7 2003-11-07 12:21:54 bert Exp $
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
				mCreateDataObj(ColorSequence);

    void			loadFromStorage( const char* nm );
			
    ColorTable&		colors();
    const ColorTable&	colors() const;
    void		colorsChanged();
			/*!< If you change the Colortable, notify me with
			     this function
			*/

    Notifier<ColorSequence>	change;

    int				usePar( const IOPar& );
    void			fillPar( IOPar&, TypeSet<int>& ) const;
protected:
    virtual		~ColorSequence();


    ColorTable&			coltab;
};

}; // Namespace


#endif

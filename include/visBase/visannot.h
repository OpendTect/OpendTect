#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.1 2002-02-06 22:30:19 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "sets.h"

class SoCoordinate3;
class SoText2;
class SoTranslation;

namespace visBase
{

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

class Annotation : public VisualObject
{
public:

				Annotation();
    void			setCorner( int, float, float, float );
    void			setText( int dim, const char * );

protected:

    void			updateTextPos();
    SoCoordinate3*		coords;

    ObjectSet<SoText2>		texts;
    ObjectSet<SoTranslation>	textpositions;

};

};

#endif

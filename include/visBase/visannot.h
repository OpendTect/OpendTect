#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.8 2002-04-25 10:37:23 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "sets.h"
#include "draw.h"

class SoSwitch;
class SoDrawStyle;
class SoCoordinate3;
class AxisInfo;

namespace visBase
{
class Text;
class SceneObjectGroup;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

class Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj0arg(Annotation);

    void			showText( bool yn );
    bool			isTextShown() const;

    void			setCorner( int, float, float, float );
    void			setText( int dim, const char * );
    void			setLineStyle( const LineStyle& );
    				/*!< Color in Linestyle is ignored, must be
				     set separately 
				*/
    const LineStyle&		lineStyle() const { return linestyle; }

protected:
    				~Annotation();
    LineStyle			linestyle;
    void			updateLineStyle();

    void			updateTextPos(int dim);
    void			updateTextPos();
    Text*			getText( int dim, int textnr );

    SoCoordinate3*		coords;

    ObjectSet<SceneObjectGroup>	texts;
    
    SoSwitch*			textswitch;
};

};

#endif

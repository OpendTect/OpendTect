#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.10 2002-04-29 09:26:21 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "sets.h"

class SoSwitch;
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

    void			showScale( bool yn );
    bool			isScaleShown() const;

    void			setCorner( int, float, float, float );
    void			setText( int dim, const char * );

protected:
    				~Annotation();
    void			updateTextPos(int dim);
    void			updateTextPos();
    Text*			getText( int dim, int textnr );

    SoCoordinate3*		coords;

    ObjectSet<SceneObjectGroup>	scales;
    SceneObjectGroup*		texts;
    
    SoSwitch*			textswitch;
    SoSwitch*			scaleswitch;
};

};

#endif

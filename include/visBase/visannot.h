#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.21 2010-08-19 08:21:10 cvsranojay Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "color.h"
#include "position.h"
#include "sets.h"

class SoSwitch;
class SoCoordinate3;
class AxisInfo;
class Color;

namespace visBase
{
class Text2;
class DataObjectGroup;
class PickStyle;

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

mClass Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj(Annotation);

    void			showText( bool yn );
    bool			isTextShown() const;

    void			showScale( bool yn );
    bool			isScaleShown() const;

    void			setCorner( int, float, float, float );
    Coord3			getCorner( int ) const;
    void			setText( int dim, const char * );
    void			setTextColor(int dim,const Color&);
    const Color&		getColor()		{ return annotcolor_; }
    void			updateTextColor(const Color&);

    void			fillPar( IOPar&, TypeSet<int>& ) const;
    int				usePar( const IOPar& );

protected:
    				~Annotation();
    void			updateTextPos(int dim);
    void			updateTextPos();
    Text2*			getText( int dim, int textnr );

    SoCoordinate3*		coords;

    ObjectSet<DataObjectGroup>	scales;
    PickStyle*			pickstyle;
    DataObjectGroup*		texts;
    
    SoSwitch*			textswitch;
    SoSwitch*			scaleswitch;
    Color			annotcolor_;

    static const char*		textprefixstr();
    static const char*		cornerprefixstr();
    static const char*		showtextstr();
    static const char*		showscalestr();
};

};

#endif

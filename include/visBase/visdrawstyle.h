#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdrawstyle.h,v 1.1 2002-04-25 13:42:13 kristofer Exp $
________________________________________________________________________

-*/

#include "vissceneobj.h"
#include "draw.h"

class SoDrawStyle;

namespace visBase
{
/*! \brief
*/


class DrawStyle : public SceneObject
{
public:
    static DrawStyle*	create()
			mCreateDataObj0arg(DrawStyle);

    enum Style		{ Filled, Lines, Points, Invisible };

    void		setDrawStyle( Style );
    Style		getDrawStyle() const;

    void		setPointSize( float );
    float		getPointSize() const;

    void		setLineStyle( const LineStyle& );
    			/*!< Color in Linestyle is ignored, must be
			     set separately.
			*/
    const LineStyle&	lineStyle() const { return linestyle; }

    SoNode*		getData();
private:
    virtual		~DrawStyle();

    void		updateLineStyle();
    
    LineStyle		linestyle;


    SoDrawStyle*	drawstyle;
};

};


#endif

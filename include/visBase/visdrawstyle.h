#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdrawstyle.h,v 1.2 2002-04-29 08:45:58 kristofer Exp $
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
    			DeclareEnumUtils(Style);

    void		setDrawStyle( Style );
    Style		getDrawStyle() const;

    void		setPointSize( float );
    float		getPointSize() const;

    void		setLineStyle( const LineStyle& );
    			/*!< Color in Linestyle is ignored, must be
			     set separately.
			*/
    const LineStyle&	lineStyle() const { return linestyle; }

    int			usePar( const IOPar& );
    void		fillPar( IOPar& ) const;

    SoNode*		getData();
private:
    virtual		~DrawStyle();

    void		updateLineStyle();
    
    LineStyle		linestyle;
    SoDrawStyle*	drawstyle;

    static const char*	linestylestr;
    static const char*	drawstylestr;
    static const char*	pointsizestr;
};

};


#endif

#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdrawstyle.h,v 1.5 2003-11-07 12:21:54 bert Exp $
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
			mCreateDataObj(DrawStyle);

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
    void		fillPar( IOPar&, TypeSet<int>& ) const;

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

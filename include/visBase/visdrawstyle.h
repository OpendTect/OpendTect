#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdrawstyle.h,v 1.7 2008-12-17 13:08:34 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visdata.h"
#include "draw.h"

class SoDrawStyle;

namespace visBase
{
/*! \brief
*/


class DrawStyle : public DataObject
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
    void 		setLineWidth(int);
    const LineStyle&	lineStyle() const { return linestyle; }

    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

    SoNode*		getInventorNode();
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

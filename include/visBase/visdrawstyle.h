#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visdrawstyle.h,v 1.9 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "visdata.h"
#include "draw.h"

class SoDrawStyle;

namespace visBase
{
/*! \brief
*/


mClass DrawStyle : public DataObject
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

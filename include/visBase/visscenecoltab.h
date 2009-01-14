#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.7 2009-01-14 11:40:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "visobject.h"

class LegendKit;
class Color;
namespace ColTab { class MapperSetup; class Sequence; }

namespace visBase
{

mClass VisColorTab;

mClass SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*	create()
			mCreateDataObj(SceneColTab);

    void		setLegendColor(const Color&);
    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    void		setDisplayTransformation(Transformation*) {}

protected:
			~SceneColTab();

    LegendKit*		legendkit_;
};

} // class visBase


#endif

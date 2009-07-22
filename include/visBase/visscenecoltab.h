#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.9 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "visobject.h"

class LegendKit;
class Color;
namespace ColTab { struct MapperSetup; class Sequence; }

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

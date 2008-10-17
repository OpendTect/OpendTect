#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.5 2008-10-17 05:05:26 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visobject.h"

class LegendKit;
namespace ColTab { class MapperSetup; class Sequence; }

namespace visBase
{

class VisColorTab;

class SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*	create()
			mCreateDataObj(SceneColTab);

    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    void		setDisplayTransformation(Transformation*) {}

protected:
			~SceneColTab();

    LegendKit*		legendkit_;
};

} // class visBase


#endif

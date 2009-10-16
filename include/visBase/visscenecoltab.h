#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.11 2009-10-16 05:49:17 cvsranojay Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "coltabsequence.h"

class LegendKit;
class Color;
namespace ColTab { struct MapperSetup; }

namespace visBase
{

mClass VisColorTab;

mClass SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*		create()
				mCreateDataObj(SceneColTab);

    void			setLegendColor(const Color&);
    void			setColTabSequence(const ColTab::Sequence&);
    void			setColTabMapperSetup(const ColTab::MapperSetup&);

    void			setDisplayTransformation(Transformation*) {}
    void			turnOn(bool);
    void			setSize(int w,int h);
    Geom::Size2D<int>		getSize();
    void			setPos(bool top,bool left); 


protected:
				~SceneColTab();
    void			updateVis();			

    LegendKit*			legendkit_;
    ColTab::Sequence		sequence_;
    Interval<float>		rg_;
};

} // class visBase


#endif

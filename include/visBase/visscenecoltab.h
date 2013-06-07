#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visobject.h"
#include "coltabsequence.h"

class LegendKit;
class Color;
namespace ColTab { class MapperSetup; }

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

    void			setDisplayTransformation(const mVisTrans*) {}
    void			turnOn(bool);
    void			setSize(int w,int h);
    Geom::Size2D<int>		getSize();

    enum Pos			{ BottomLeft, BottomRight, TopLeft, TopRight };
    void			setPos( Pos pos ); 
    Pos				getPos() const	    { return pos_; }

protected:
				~SceneColTab();
    void			updateVis();			

    LegendKit*			legendkit_;
    ColTab::Sequence		sequence_;
    Interval<float>		rg_;
    Pos				pos_;
    bool			flipseq_;
};

} // class visBase


#endif

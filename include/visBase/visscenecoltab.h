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

#include "visbasemod.h"
#include "visobject.h"
#include "coltabsequence.h"

class LegendKit;
namespace ColTab { class MapperSetup; }
namespace osg { class Geode; }

namespace visBase
{

class VisColorTab;

mExpClass(visBase) SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*	create()
			mCreateDataObj(SceneColTab);

    void		setLegendColor(const Color&);
    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);

    void		setDisplayTransformation(const mVisTrans*) {}
    bool		turnOn(bool);
    void		setSize(int w,int h);
    Geom::Size2D<int>	getSize();

    void		setOrientation(bool horizontal);
    bool		getOrientation() const { return horizontal_; }

    enum Pos		{ BottomLeft, BottomRight, TopLeft, TopRight };
    void		setPos( Pos pos );
    Pos			getPos() const	    { return pos_; }

    void		setPos(float x, float y);

protected:
			~SceneColTab();
    void		updateVis();

    osg::Geode*		geode_;
    ColTab::Sequence	sequence_;
    Interval<float>	rg_;
    Pos			pos_;
    bool		flipseq_;
    bool		horizontal_;
};

} // class visBase


#endif


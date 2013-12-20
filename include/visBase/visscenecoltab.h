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

class FontData;
namespace ColTab { class MapperSetup; }
namespace osgGeo { class ScalarBar; }

namespace visBase
{

class VisColorTab;

mExpClass(visBase) SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*	create()
			mCreateDataObj(SceneColTab);

    enum Pos		{ Left, Right, Top, Bottom };
    void		setPos( Pos pos );
    Pos			getPos() const	    { return pos_; }
    
    void		setWindowSize(int winx, int winy);

    void		setOrientation(bool horizontal);
    bool		getOrientation() const { return horizontal_; }

    void		setAnnotFont(const FontData&); 
       
    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);
    void		setLegendColor(const Color&);

    bool		turnOn(bool);
    void		setSize(int w,int h);
    Geom::Size2D<int>	getSize();
        
protected:
			~SceneColTab();
    void		updateSequence();
    void		setPos(float x, float y);

    osgGeo::ScalarBar*	osgcolorbar_;
    ColTab::Sequence	sequence_;
    Interval<float>	rg_;
    Pos			pos_;
    bool		flipseq_;
    bool		horizontal_;
    int			width_;
    int			height_;
    float		aspratio_;
    int			winx_;
    int			winy_;
};

} // class visBase


#endif


#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2008
________________________________________________________________________

-*/

#include "visobject.h"
#include "coltabsequence.h"

class FontData;
namespace osg { class Geode; }


namespace visBase
{

class VisColorTab;

mExpClass(visBase) SceneColTab : public VisualObjectImpl
{ mODTextTranslationClass(SceneColTab);
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
    void		setColTabMapper(const ColTab::Mapper&);
    void		setLegendColor(const Color&);

    bool		turnOn(bool);
    void		setSize(int w,int h);
    Geom::Size2D<int>	getSize();

    virtual void	setPixelDensity(float dpi);


protected:

			~SceneColTab();
    void		updateDisplay();
    void		setPos(float x, float y);
    void		coltabChgCB( CallBacker* )	{ updateDisplay(); }

    osg::Geode*		osgcolorbar_;
    ConstRefMan<ColTab::Sequence> sequence_;
    ConstRefMan<ColTab::Mapper> mapper_;
    Pos			pos_;
    bool		horizontal_;
    int			width_;
    int			height_;
    float		aspratio_;
    int			winx_;
    int			winy_;
    float		pixeldensity_;
    int			fontsize_;

};

} // class visBase

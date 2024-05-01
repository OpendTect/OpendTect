#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "coltabsequence.h"

class FontData;
namespace ColTab { class MapperSetup; }

namespace osg { class Geode; }


namespace visBase
{

class VisColorTab;

mExpClass(visBase) SceneColTab : public VisualObjectImpl
{ mODTextTranslationClass(SceneColTab);
public:
    static RefMan<SceneColTab> create();
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
    void		setLegendColor(const OD::Color&);

    bool		turnOn(bool) override;
    void		setSize(int w,int h);
    Geom::Size2D<int>	getSize();

    void		setPixelDensity(float dpi) override;

    const ColTab::Sequence& getColTabSequence() const { return sequence_; }
    bool		isSeqFlipped() const { return flipseq_; }
    const Interval<float>& getRange() const { return rg_; }

protected:
			~SceneColTab();
    void		updateSequence();
    void		setPos(float x, float y);

    osg::Geode*		osgcolorbar_;
    ColTab::Sequence	sequence_;
    Interval<float>	rg_;
    Pos			pos_		= Pos::Bottom;
    bool		flipseq_	= false;
    bool		horizontal_	= false;
    int			width_		= 20;
    int			height_		= 250;
    float		aspratio_	= 1.f;
    int			winx_		= 100;
    int			winy_		= 100;
    float		pixeldensity_;
    int			fontsize_	= 18;
};

} // namespace visBase

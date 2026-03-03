#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visobject.h"
#include "coltabsequence.h"
#include <osgGeo/ScalarBar>

class FontData;
class VisColorTab;

namespace ColTab { class MapperSetup; }

namespace osg { class Geode; }


struct ColorBarBounds {
    // Horizontal constraints
    static constexpr int minHorWidth()	    { return 350; }
    static constexpr int maxHorWidth()	    { return 1000; }
    static constexpr int minHorHeight()     { return 20; }
    static constexpr int maxHorHeight()     { return 35; }

    // Vertical constraints
    static constexpr int minVertWidth()     { return 20; }
    static constexpr int maxVertWidth()     { return 80; }
    static constexpr int minVertHeight()    { return 350; }
    static constexpr int maxVertHeight()    { return 800; }
};


namespace visBase
{
mExpClass(visBase) SceneColTab : public VisualObjectImpl
{ mODTextTranslationClass(SceneColTab);
public:
    static RefMan<SceneColTab> create();
			mCreateDataObj(SceneColTab);

    enum Pos		{ Left, Top, Right, Bottom };
    void		setPos( Pos pos );
    Pos			getPos() const			{ return pos_; }

    void		setWindowSize(int winx, int winy);

    void		setOrientation(bool horizontal);
    bool		getOrientation() const		{ return horizontal_; }

    void		setAnnotFont(const FontData&);

    void		setColTabSequence(const ColTab::Sequence&);
    void		setColTabMapperSetup(const ColTab::MapperSetup&);
    void		setLegendColor(const OD::Color&);

    bool		turnOn(bool) override;
    void		setSize(int width,int height);
    Geom::Size2D<int>	getSize();

    int			getLabelCharSize();

    int			getMinLabelWidth();
    int			getMinLabelHeight();

    void		setPixelDensity(float dpi) override;

    const ColTab::Sequence& getColTabSequence() const	{ return sequence_; }
    bool		isSeqFlipped() const		{ return flipseq_; }
    const Interval<float>& getRange() const		{ return rg_; }
    int			getNumLabels();

protected:
			~SceneColTab();
    void		updateSequence();
    void		setPos(float x, float y);
    void		setNumLabels(int numoflbls);

    osgGeo::ScalarBar*	osgcolorbar_;
    ColTab::Sequence	sequence_;
    Interval<float>	rg_;
    Pos			pos_		= Pos::Bottom;
    bool		flipseq_	= false;
    bool		horizontal_	= false;
    int			width_		= 20;
    int			height_		= 500;
    float		aspratio_	= 1.f;
    int			winx_		= 100;
    int			winy_		= 100;
    float		pixeldensity_;
    int			fontsize_	= 18;
};

} // namespace visBase

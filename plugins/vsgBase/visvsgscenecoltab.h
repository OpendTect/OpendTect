#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "visvsgdata.h"

#include "color.h"
#include "coltabsequence.h"
#include "fontdata.h"
#include "geometry.h"
#include "ranges.h"

#include <vsg/core/ref_ptr.h>

namespace ColTab { class MapperSetup; }
namespace vsgGeo { class ScalarBar; }

namespace dGBVis
{

/*!\brief The limits of the size of the color bar in pixels, as in OpendTect */

struct ColorBarBounds
{
    static constexpr int	minHorWidth()	{ return 350; }
    static constexpr int	maxHorWidth()	{ return 1000; }
    static constexpr int	minHorHeight()	{ return 20; }
    static constexpr int	maxHorHeight()	{ return 35; }
    static constexpr int	minVertWidth()	{ return 20; }
    static constexpr int	maxVertWidth()	{ return 80; }
    static constexpr int	minVertHeight()	{ return 350; }
    static constexpr int	maxVertHeight()	{ return 800; }
};


/*!\brief Counterpart of visBase::SceneColTab, the color bar of the scene,
  placed at the left, top, right or bottom of the window. For a HUD. */

mExpClass(vsgBase) SceneColTab : public DataObject
{
public:
    static RefMan<SceneColTab>	create();
				mCreateDGBVisObj(SceneColTab);

    enum Pos			{ Left, Top, Right, Bottom };
    void			setPos(Pos);
    Pos				getPos() const		{ return pos_; }
				/*!True for a bar at the top or the bottom */
    bool			getOrientation() const	{ return horizontal_; }

    void			setWindowSize(int winx,int winy);

    void			setSize(int width,int height);
    Geom::Size2D<int>		getSize() const;
    int				getLength() const	{ return length_; }
    int				getThickness() const	{ return thickness_; }

    void			setAnnotFont(const FontData&);
    void			setColTabSequence(const ColTab::Sequence&);
    void			setColTabMapperSetup(
					const ColTab::MapperSetup&);
    void			setLegendColor(const OD::Color&);

    bool			turnOn(bool) override;
    void			setPixelDensity(float) override;

    const ColTab::Sequence&	getColTabSequence() const { return sequence_; }
    bool			isSeqFlipped() const	{ return flipseq_; }
    const Interval<float>&	getRange() const	{ return rg_; }
    int				getNumLabels() const;
    int				getLabelCharSize() const;
    int				getMinLabelWidth() const;
    int				getMinLabelHeight() const;

    vsgGeo::ScalarBar*		vsgBar()		{ return bar_.get(); }

protected:
				~SceneColTab();

    vsg::ref_ptr<vsgGeo::ScalarBar> bar_;

private:

    void			prepare();
    void			updateSequence();
    void			setNumLabels(int);
    void			applySize();
    void			placeBar();
    void			fontLoadedCB(CallBacker*);

    ColTab::Sequence		sequence_;
    Interval<float>		rg_;
    bool			flipseq_ = false;
    Pos				pos_ = Bottom;
    bool			horizontal_ = true;
    int				length_ = 500;
    int				thickness_ = 20;
    int				winx_ = 100;
    int				winy_ = 100;
    FontData			fontdata_;
    float			pixeldensity_;
};

} // namespace dGBVis

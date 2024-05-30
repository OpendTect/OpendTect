#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "visobject.h"

namespace Pick { class Location; }

namespace visBase
{
    class DrawStyle;
    class Lines;
    class MarkerSet;
    class DataObjectGroup;
    class Transformation;

/*!\brief
  ScaleBar base object
*/

mExpClass(visBase) ScaleBar : public visBase::VisualObjectImpl
{
public:
    static RefMan<ScaleBar>	create();
				mCreateDataObj(ScaleBar);

    void			setPick(const Pick::Location&);

    void			setLineWidth(int);
    void			setLength(double);

    void			setOnInlCrl(bool);
    void			setOrientation(int);
    void			setColor(OD::Color c);
    Coord3			getPos() const { return pos_; }

    void			setDisplayTransformation(
						const mVisTrans*) override;

protected:
				~ScaleBar();

    Coord3			getSecondPos(const Pick::Location&) const;
    void			updateVis(const Pick::Location&);

    RefMan<visBase::MarkerSet>	markers_;
    RefMan<visBase::Lines>	lines_;
    RefMan<visBase::DrawStyle>	linestyle_;
    ConstRefMan<mVisTrans>	displaytrans_;

    bool			oninlcrl_ = true;
    double			length_ = 1000.;
    int				orientation_ = 0;
    Coord3			pos_;
    Pick::Location&		firstloc_;
};

} // namespace visBase

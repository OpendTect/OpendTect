#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
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
    static ScaleBar*		create()
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

    visBase::MarkerSet*		markers_;
    visBase::Lines*		lines_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytrans_;

    bool			oninlcrl_;
    double			length_;
    int				orientation_;
    Coord3			pos_;
    Pick::Location&		firstloc_;
};

} // namespace visBase

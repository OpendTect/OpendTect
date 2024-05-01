#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "trckeyzsampling.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistransform.h"

namespace OD { class LineStyle; }

namespace visBase
{

/*!\brief
*/

mExpClass(visBase) GridLines : public VisualObjectImpl
{
public:
    static RefMan<GridLines>	create();
				mCreateDataObj(GridLines);

    void			setDisplayTransformation(
						const mVisTrans*) override;

    void			setLineStyle(const OD::LineStyle&);
    void			getLineStyle(OD::LineStyle&) const;

    void			adjustGridCS();
    void			setGridTrcKeyZSampling(const TrcKeyZSampling&);
    void			setPlaneTrcKeyZSampling(const TrcKeyZSampling&);
    const TrcKeyZSampling&	getGridTrcKeyZSampling()  { return gridcs_; }
    const TrcKeyZSampling&	getPlaneTrcKeyZSampling() { return planecs_; }

    void			showInlines(bool);
    bool			areInlinesShown() const;
    void			showCrosslines(bool);
    bool			areCrosslinesShown() const;
    void			showZlines(bool);
    bool			areZlinesShown() const;
    void			setPixelDensity(float) override;

protected:
				~GridLines();

    TrcKeyZSampling		gridcs_;
    TrcKeyZSampling		planecs_;
    bool			csinlchanged_ = false;
    bool			cscrlchanged_ = false;
    bool			cszchanged_ = false;

    RefMan<PolyLine>		inlines_;
    RefMan<PolyLine>		crosslines_;
    RefMan<PolyLine>		zlines_;
    RefMan<PolyLine>		trcnrlines_;

    RefObjectSet<PolyLine>	polylineset_;
    RefMan<DrawStyle>		drawstyle_;
    ConstRefMan<mVisTrans>	transformation_;
    RefMan<Material>		linematerial_;

    void			emptyLineSet(PolyLine*);
    RefMan<PolyLine>		addLineSet();
    void			addLine(PolyLine&,const Coord3& start,
					const Coord3& stop);

    void			drawInlines();
    void			drawCrosslines();
    void			drawZlines();

    static const char*		sKeyLineStyle();
    static const char*		sKeyInlShown();
    static const char*		sKeyCrlShown();
    static const char*		sKeyZShown();
};

} // namespace visBase

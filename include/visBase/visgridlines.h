#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "trckeyzsampling.h"

namespace OD { class LineStyle; }

namespace visBase
{

/*!\brief
*/

class DrawStyle;
class PolyLine;
class Transformation;

mExpClass(visBase) GridLines : public VisualObjectImpl
{
public:
    static GridLines*		create()
				mCreateDataObj(GridLines);
				~GridLines();

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

    TrcKeyZSampling		gridcs_;
    TrcKeyZSampling		planecs_;
    bool			csinlchanged_;
    bool			cscrlchanged_;
    bool			cszchanged_;

    PolyLine*			inlines_;
    PolyLine*			crosslines_;
    PolyLine*			zlines_;
    PolyLine*			trcnrlines_;

    ObjectSet<PolyLine>		polylineset_;
    DrawStyle*			drawstyle_;
    const  mVisTrans*		transformation_;
    Material*			linematerial_;

    void			emptyLineSet(PolyLine*);
    PolyLine*			addLineSet();
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

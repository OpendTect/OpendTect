#ifndef visscalebar_h
#define visscalebar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
 RCS:		$Id: visscalebar.h,v 1.1 2012-04-02 22:39:38 cvsnanne Exp $
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase { class DrawStyle; class IndexedPolyLine; class Marker; }

namespace Annotations
{

/*!\brief
  ScaleBar base object
*/

class ScaleBar : public visBase::VisualObjectImpl
{
public:
    static ScaleBar*		create()
				mCreateDataObj(ScaleBar);

    void			setPick(const Pick::Location&);
    void			setDisplayTransformation(const mVisTrans*);
    void			setLineWidth(int);

protected:
				~ScaleBar();

    visBase::Marker*		marker1_;
    visBase::Marker*		marker2_;
    visBase::IndexedPolyLine*	polyline_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytrans_;
};

/*!\brief
  ScaleBar Display
*/

class ScaleBarDisplay : public visSurvey::LocationDisplay
{
public:
    static ScaleBarDisplay*	create()
				mCreateDataObj(ScaleBarDisplay);

    void			setScene(visSurvey::Scene*);

    enum Orientation		{ Horizontal, Vertical };
    void			setOrientation(Orientation);
    Orientation			getOrientation() const;

    void			setLineWidth(int);
    int				getLineWidth() const;

protected:
    				~ScaleBarDisplay();

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);
    visBase::VisualObject*	createLocation() const;
    void			setPosition(int,const Pick::Location&);
    bool			hasDirection() const { return true; }

    Orientation			orientation_;
    int				linewidth_;
};


} // namespace Annotations

#endif

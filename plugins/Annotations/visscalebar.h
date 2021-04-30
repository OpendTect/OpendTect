#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		January 2005
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

namespace visBase { class DrawStyle; class IndexedPolyLine; class Marker; }

namespace Annotations
{

/*!\brief
  ScaleBar base object
*/

mClass(Annotations) ScaleBar : public visBase::VisualObjectImpl
{
public:
    static ScaleBar*		create()
				mCreateDataObj(ScaleBar);

    void			setPick(const Pick::Location&);
    void			setDisplayTransformation(const mVisTrans*);
    void			setLineWidth(int);
    void			setLength(double);
    void			setOnInlCrl(bool);
    void			setOrientation(int);

protected:
				~ScaleBar();

    Coord3			getSecondPos(const Pick::Location&) const;
    void			updateVis(const Pick::Location&);

    visBase::Marker*		marker1_;
    visBase::Marker*		marker2_;
    visBase::IndexedPolyLine*	polyline_;
    visBase::DrawStyle*		linestyle_;
    const mVisTrans*		displaytrans_;

    bool			oninlcrl_;
    double			length_;
    int				orientation_;
    Pick::Location&		firstloc_;
};

/*!\brief
  ScaleBar Display
*/

mClass(Annotations) ScaleBarDisplay : public visSurvey::LocationDisplay
{
public:
    static ScaleBarDisplay*	create()
				mCreateDataObj(ScaleBarDisplay);
    				~ScaleBarDisplay();

    void			setScene(visSurvey::Scene*);

    void			setOnInlCrl(bool);
    bool			isOnInlCrl() const;
    void			setOrientation(int);
    int				getOrientation() const;

    void			setLineWidth(int);
    int				getLineWidth() const;
    void			setLength(double);
    double			getLength() const;

    void			fromPar(const IOPar&);
    void			toPar(IOPar&) const;

protected:

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);
    visBase::VisualObject*	createLocation() const;
    void			setPosition(int,const Pick::Location&);
    int				isMarkerClick(const TypeSet<int>&) const;
    bool			hasDirection() const { return false; }

    bool			oninlcrl_;
    int				orientation_;
    int				linewidth_;
    double			length_;
};


} // namespace Annotations


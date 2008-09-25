#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.4 2008-09-25 09:44:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "visobject.h"

class LegendKit;
namespace ColTab { class Sequence; }

template<class T> class Interval;

namespace visBase
{

class VisColorTab;

class SceneColTab : public VisualObjectImpl
{
public:
    static SceneColTab*		create()
				mCreateDataObj(SceneColTab);

    void			setColTabID(int);
    void			setColTabSequence(const ColTab::Sequence&);
    void			setRange(const Interval<float>&);

    void			setDisplayTransformation(Transformation*) {}

protected:
				~SceneColTab();

    void			rangeChg(CallBacker*);
    void			seqChg(CallBacker*);

    LegendKit*			legendkit_;
    VisColorTab*		viscoltab_;
};

} // class visBase


#endif

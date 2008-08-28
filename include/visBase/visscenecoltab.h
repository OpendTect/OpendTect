#ifndef visscenecoltab_h
#define visscenecoltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2008
 RCS:		$Id: visscenecoltab.h,v 1.1 2008-08-28 08:53:30 cvsnanne Exp $
________________________________________________________________________

-*/

class LegendKit;
namespace ColTab { class Sequence; }

template<class T> class Interval;

namespace visBase
{

class SceneColTab : public DataObject
{
public:
    static SceneColTab*		create()
				mCreateDataObj(SceneColTab);

    void			setColTabSequence(const ColTab::Sequence&);
    void			setRange(const Interval<float>&);

protected:
				~SceneColTab();

    LegendKit*			legendkit_;
};

} // class visBase


#endif

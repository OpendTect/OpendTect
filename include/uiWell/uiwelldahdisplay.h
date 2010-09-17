#ifndef uiwelldahdisplay_h
#define uiwelldahdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2010
 RCS:           $Id: uiwelldahdisplay.h,v 1.1 2010-09-17 12:26:07 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "welldisp.h"
#include "survinfo.h"

namespace Well { class Log; class Marker; class D2TModel; }

mClass uiWellDahDisplay : public uiGraphicsView
{
public:	

    mStruct Data
    {
				Data()
				    : zrg_(mUdf(float),0)
				    , zistime_(false)
				    , dispzinft_(SI().depthsInFeetByDefault())
				    , markers_(0)
				    , d2tm_(0)
				    {}

	void copyFrom(const Data& d)
	{
	    zrg_     	= d.zrg_;
	    zistime_ 	= d.zistime_;
	    dispzinft_ 	= d.dispzinft_;
	    markers_ 	= d.markers_;
	    d2tm_    	= d.d2tm_;
	}

	Interval<float>		zrg_;
	bool			dispzinft_;
	bool			zistime_;
	const Well::D2TModel*	d2tm_;
	const ObjectSet<Well::Marker>* markers_;
    };	

    void			setMarkers(const ObjectSet<Well::Marker>* ms)
				{ zdata_.markers_ = ms; dataChanged(); }
    void			setZRange(Interval<float> zrg)
				{ zdata_.zrg_ = zrg; dataChanged();}
    void			setData(const Data& data)
				{ zdata_.copyFrom(data); dataChanged();}

    const Data&			zData() { return zdata_; }

protected:
    				uiWellDahDisplay(uiParent*,const char*);

    virtual void		draw() = 0;
    virtual void		gatherInfo() = 0;
    void			dataChanged();
    Data			zdata_;

    void			init(CallBacker*);
    void			reSized(CallBacker*);
};


#endif

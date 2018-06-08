#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2018
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include "datapack.h"

class uiHistogramDisplay;
class uiManipHandleItem;
class uiTextItem;
class uiAxisHandler;

template <class T> class Array2D;

mExpClass(uiTools) uiHistogramSel : public uiGroup
{
public:
				uiHistogramSel(uiParent*,int an_id=0,
						    bool fixdrawrg=true);
				~uiHistogramSel();

    int				ID() const		{ return id_; }
    void			setID( int id )		{ id_ = id; }

    void			setEmpty();
    bool			setDataPackID(DataPack::ID,DataPackMgr::ID,
					      int version);
    void			setData(const Array2D<float>*);
    void			setData(const float*,od_int64 sz);
    bool			setData(const IOPar&);
    void			setMarkValue(float,bool forx);

    const Interval<float>&	getDataRange() const	{ return datarg_; }
    void			setDataRange(const Interval<float>&);
    const Interval<float>&	getSelRange() const	{ return cliprg_; }
    void			setSelRange(const Interval<float>&);

    uiHistogramDisplay&		getDisplay()	{ return *histogramdisp_; }

    Notifier<uiHistogramSel>	rangeChanged;

protected:

    int				id_;
    uiHistogramDisplay*		histogramdisp_;
    uiAxisHandler*		xax_;

    uiManipHandleItem*		minhandle_;
    uiManipHandleItem*		maxhandle_;
    uiTextItem*			minvaltext_;
    uiTextItem*			maxvaltext_;

    Interval<float>		datarg_;
    Interval<float>		cliprg_;
    int				startpix_;
    int				stoppix_;

    bool			mousedown_;

    virtual void		drawAgain();
    virtual void		useClipRange()	{}

    void			init();
    void			drawText();
    void			drawLines();
    bool			changeLinePos(bool pressedonly=false);

    void			mousePressed(CallBacker*);
    void			mouseMoved(CallBacker*);
    void			mouseReleased(CallBacker*);

    void			histogramResized(CallBacker*);
    void			histDRChanged(CallBacker*);
};

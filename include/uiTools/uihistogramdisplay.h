#ifndef uihistogramdisplay_h
#define uihistogramdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uihistogramdisplay.h,v 1.8 2009-06-09 08:18:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "datapack.h"

class uiTextItem;
template <class T> class Array2D;
namespace Stats { template <class T> class RunCalc; }

mClass uiHistogramDisplay : public uiFunctionDisplay
{
public:

    				uiHistogramDisplay(uiParent*,Setup&,
						   bool withheader=false);
				~uiHistogramDisplay();

    bool			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float*,int sz);
    void			setData(const Array2D<float>*);

    void			setHistogram(const TypeSet<float>&,
	    				     Interval<float>,int N=-1);

    const Stats::RunCalc<float>&	getRunCalc()	{ return rc_; }
    int                         nrInpVals() const       { return nrinpvals_; }
    int				nrClasses() const       { return nrclasses_; }
    void			putN(); 

protected:

    Stats::RunCalc<float>&	rc_;	
    int                         nrinpvals_;
    int                         nrclasses_;
    bool			withheader_;
    uiTextItem*			header_;
    uiTextItem*			nitm_;
    
    void			updateAndDraw();
    void			updateHistogram();
};


#endif

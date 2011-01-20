#ifndef uiwelltieeventstretch_h
#define uiwelltieeventstretch_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: uiwelltieeventstretch.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "callback.h"
#include "welltiegeocalculator.h"

namespace Well { class D2TModel; }

template <class T> class Array1DImpl;

namespace WellTie
{

class Marker;
class GeoCalculator;
class PickSetMgr;
class PickData;

mClass EventStretch : public CallBacker
{
public:
					EventStretch(PickSetMgr&);
					~EventStretch();

    void 				doWork(CallBacker*); 
    void				setD2TModel(const Well::D2TModel* d2t)
					{ d2t_ = d2t; }

    Notifier<EventStretch>		timeChanged;
    const Array1DImpl<float>*		timeArr() const { return timearr_; }
    
protected:

    PickSetMgr&				pmgr_;
    GeoCalculator       		geocalc_;
    const Well::D2TModel*		d2t_;
    const TypeSet<Marker>&      	synthpickset_;
    const TypeSet<Marker>&      	seispickset_;
    Array1DImpl<float>*			timearr_;

    float                               supborderpos_;
    float                               infborderpos_;
    float                               startpos_;
    float                               stoppos_;

    void 				drawLogsData();
    void 				doStretchWork();
    void                                doStretchData(const Array1DImpl<float>&,
							  Array1DImpl<float>&);
    void				shiftDahData(); 
    void				shiftModel(); 
    void				updateTime(float&);
    void				updatePicksPos(
	    					const Array1DImpl<float>&,
						const Array1DImpl<float>&,
						bool,int);
};

}; //namespace WellTie

#endif

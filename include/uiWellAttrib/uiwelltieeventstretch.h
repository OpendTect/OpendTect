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
#include "arrayndimpl.h"
#include "welltiedata.h"

namespace Well { class D2TModel; }

namespace WellTie
{

class DataHolder;
class D2TModelMGR;
class PickSetMGR;
class PickSet;
class Setup;
class Param;
class GeoCalculator;

mClass EventStretch : public CallBacker
{
public:
					EventStretch(WellTie::DataHolder&);
					~EventStretch(){};

    void 				doWork(CallBacker*); 
    void				setD2TModel(const Well::D2TModel*);

    Notifier<EventStretch>		timeChanged;
    
protected:

    const Well::D2TModel*		d2t_;
    WellTie::D2TModelMGR*		d2tmgr_;
    WellTie::PickSet& 			seispickset_;
    WellTie::PickSet& 			synthpickset_;
    WellTie::PickSetMGR& 		pmgr_;
    const WellTie::GeoCalculator*       geocalc_;
    const Setup&                        wtsetup_;
    const Params&                       params_;

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
						WellTie::PickSet&,int);
};

}; //namespace WellTie

#endif

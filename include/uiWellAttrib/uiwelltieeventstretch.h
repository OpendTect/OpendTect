#ifndef uiwelltieeventstretch_h
#define uiwelltieeventstretch_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: uiwelltieeventstretch.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uiwelltiestretch.h"

class WellTieSetup;
class WellTieParams;
class WellTieDataSet;
class WellTieParams;
class WellTieDataMGR;
class WellTiePickSetMGR;
class WellTiePickSet;

class uiWellTieView;

namespace Well
{
    class Data;
}

mClass uiWellTieEventStretch : public uiWellTieStretch
{
public:
			uiWellTieEventStretch(uiParent*,const WellTieParams*,
			    		 const Well::Data* wd,WellTieDataMGR&,
					 uiWellTieView&,WellTiePickSetMGR&);
			~uiWellTieEventStretch();

    Notifier<uiWellTieEventStretch> 	readyforwork;
    
    void 				doWork(CallBacker*); 
    
protected:

    WellTiePickSet& 			seispickset_;
    WellTiePickSet& 			synthpickset_;

    void 				addUserPick(CallBacker*);
    void 				drawLogsData();
    void				updatePos(float&);
};

#endif

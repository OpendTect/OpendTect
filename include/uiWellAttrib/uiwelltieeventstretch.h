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

class WellTieDataHolder;
class WellTieSetup;
class WellTieParams;
class WellTieDataSet;
class WellTieParams;
class WellTieDataMGR;
class WellTieD2TModelMGR;
class WellTiePickSetMGR;
class WellTiePickSet;

class uiWellTieView;

mClass uiWellTieEventStretch : public uiWellTieStretch
{
public:
			uiWellTieEventStretch(uiParent*,WellTieDataHolder*,
					      uiWellTieView&);
			~uiWellTieEventStretch();

    Notifier<uiWellTieEventStretch> 	pickadded;
    
    void 				doWork(CallBacker*); 
    
protected:

    WellTieD2TModelMGR*			d2tmgr_;
    WellTiePickSet& 			seispickset_;
    WellTiePickSet& 			synthpickset_;
    WellTiePickSetMGR& 			pmgr_;

    void 				addSyntPick(CallBacker*);
    void 				addSeisPick(CallBacker*);
    void 				drawLogsData();
    void 				doStretchWork();
    void				shiftDahData(); 
    void				shiftModel(); 
    void				updateTime(float&);
    void				updatePicksPos(
	    					const Array1DImpl<float>&,
						const Array1DImpl<float>&,
						WellTiePickSet&,int);
};

#endif

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

#include "uiwelltiestretch.h"

namespace WellTie
{

class DataHolder;
class D2TModelMGR;
class PickSetMGR;
class PickSet;
class uiTieView;

mClass uiEventStretch : public uiStretch
{
public:
			uiEventStretch(uiParent*,WellTie::DataHolder*,
				   WellTie::uiTieView&);
			~uiEventStretch();

    Notifier<uiEventStretch> 		pickadded;
    void 				doWork(CallBacker*); 
    
protected:

    WellTie::D2TModelMGR*		d2tmgr_;
    WellTie::PickSet& 			seispickset_;
    WellTie::PickSet& 			synthpickset_;
    WellTie::PickSetMGR& 		pmgr_;

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
						WellTie::PickSet&,int);
};

}; //namespace WellTie

#endif

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

#include "uiwellattribmod.h"
#include "callback.h"

namespace Well { class D2TModel; class Track;}

namespace WellTie
{

class Marker;
class PickSetMgr;
class D2TModelMgr;

mExpClass(uiWellAttrib) EventStretch : public CallBacker
{
public:
					EventStretch(PickSetMgr&,D2TModelMgr&);

    void 				doWork(CallBacker*); 
    void				setD2TModel(const Well::D2TModel* d2t)
					{ d2t_ = d2t; }
    void				setTrack(const Well::Track* track)
					{ track_ = track; }
protected:

    PickSetMgr&				pmgr_;
    D2TModelMgr&			d2tmgr_;
    const Well::D2TModel*		d2t_;
    const Well::Track*			track_;

    const TypeSet<Marker>&      	synthpickset_;
    const TypeSet<Marker>&      	seispickset_;

    void 				doStretchWork();
    void				doStaticShift();
    void				doStretchSqueeze();
};

}; //namespace WellTie

#endif


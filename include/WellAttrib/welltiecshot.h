#ifndef welltiecshot_h
#define welltiecshot_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
Author:        Bruno
Date:          January 2009
RCS:           $Id: welltiecshot.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"

class WellTieSetup;
class DataPointSet;
namespace Well
{
    class Data;
    class Log;
    class D2TModel;
}

class uiFileInput;
class uiFunctionDisplay;
class uiGenInput;
class uiLabel;


mClass WellTieCSCorr  
{
public:
		    WellTieCSCorr(Well::Data&,const WellTieSetup&),
		    ~WellTieCSCorr() {};			

    const Well::Log& 		corrLog()              { return *log_; }


protected:

    Well::Log* 			log_;
    Well::D2TModel* 		cs_;

    void 			setCSToLogScale(TypeSet<float>&,double);
    void 			fitCS( const TypeSet<float>& csvals);
};



/*
mClass uiWellTieCShotImpDlg : public uiDialog
{
public:
		    uiWellTieCShotImpDlg(uiParent*,WellTieSetup&);
		    ~uiWellTieCShotImpDlg();			

    
    const Well::D2TModel& getCheckShot() const 	{ return *checkshot_; }


protected:

    WellTieSetup&	twtss_;
    Well::D2TModel*     checkshot_;
    uiFileInput*        filefld_;
    uiGenInput*         tvdfld_;
    uiLabel*            uilbl_;
    uiGenInput*         unitfld_;
    uiGenInput*         twtfld_;

    BufferString        fnm_;

    bool 		acceptOK(CallBacker*);
};
*/
#endif



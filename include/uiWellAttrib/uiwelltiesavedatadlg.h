#ifndef uiwelltiesavedatadlg_h
#define uiwelltiesavedatadlg_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwellwelltiesavedatadlg.h,v 1.1 2009-09-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "welltieunitfactors.h"

class CtxtIOObj;
class IOObj;
class IOObjSel;
class uiListBox;
class uiIOObjSel;
class uiTable;
class uiLabel;
class uiCheckBox;
class Wavelet;

namespace Well
{
    class Log;
}

namespace WellTie
{
    class DataHolder;
    class Log;

mClass uiSaveDataTable : public uiGroup
{
public:

      mClass Setup
      {
	      public:
				Setup()
				    : nrtimes_(0)
				    , colnm_("Log")   
				    {}	  
			
        mDefSetupMemb(BufferString,colnm)
        mDefSetupMemb(BufferStringSet,itemnames)
        mDefSetupMemb(int,nrtimes)
      };

    				uiSaveDataTable(uiParent*,CtxtIOObj&,
						const Setup&);
    				~uiSaveDataTable(){};

    bool 			getNamesToBeSaved(BufferStringSet&);
    const int			indexOf( const char* nm ) const
				{ return names_.indexOf(nm); }

protected:

    uiTable* 			table_;
    ObjectSet<uiLabel> 		labelsfld_;
    CtxtIOObj&          	ctio_;
    ObjectSet<uiCheckBox> 	chckboxfld_;
    ObjectSet<uiIOObjSel>  	ioobjselflds_;
    const BufferStringSet	names_;
    int 			nrtimessaved_;

    void			initTable();
};


mClass uiSaveDataDlg : public uiDialog
{
public: 
				uiSaveDataDlg(uiParent*,WellTie::DataHolder*);
				~uiSaveDataDlg(){};

    CtxtIOObj&          	wellctio_;
    CtxtIOObj&          	wvltctio_; 

    const WellTie::DataHolder* 	dataholder_;
    uiSaveDataTable* 		logstablefld_;
    uiSaveDataTable* 		wvltstablefld_;
    int 			nrtimessaved_;

protected:

    bool 			acceptOK(CallBacker*);
};

}; //namespace WellTie

#endif


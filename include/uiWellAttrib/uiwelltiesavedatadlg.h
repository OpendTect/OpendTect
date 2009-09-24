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
#include "bufstringset.h"
#include "welltieunitfactors.h"

class uiListBox;
class uiIOObjSel;
class uiTable;

namespace WellTie
{
    class DataHolder;

mClass uiSaveDataDlg : public uiDialog
{
public: 
			uiSaveDataDlg(uiParent*,const WellTie::DataHolder*);
			~uiSaveDataDlg(){};


protected:

    ObjectSet<uiTable> 	tableset_;
    BufferStringSet	nameslist_;
    ObjectSet<uiIOObjSel> ioobjselset_;

    const WellTie::Params::DataParams& params_;

    void 		selDone(CallBacker*);    

/*
    void                fillListBox();
    void		createSelectButtons();
    const TypeSet<MultiID>&     getSelWells() const { return selwellsids_; }*/

};

}; //namespace WellTie

#endif


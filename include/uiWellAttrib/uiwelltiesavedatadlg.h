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

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "welltieunitfactors.h"

class CtxtIOObj;
class IOObj;
class IOObjSel;
class uiGenInput;
class uiCheckBox;
class uiListBox;
class uiIOObjSel;
class uiLabel;
class uiLabeledSpinBox;
class Wavelet;

namespace Well
{
    class Log;
    class LogSet;
}

namespace WellTie
{
    class Data;
    class DataWriter;

mClass(uiWellAttrib) uiSaveDataGroup : public uiGroup
{
public:

      mClass(uiWellAttrib) Setup
      {
	  public:
				Setup()
				    : labelcolnm_("Log")   
				    , saveasioobj_(false)
				    {}	  
			
	    mDefSetupMemb(BufferString,labelcolnm)
	    mDefSetupMemb(BufferStringSet,itemnames)
	    mDefSetupMemb(bool,saveasioobj)
	    mDefSetupMemb(BufferString,wellname)
	    mDefSetupMemb(ObjectSet<CtxtIOObj>,ctio);
      };

    				uiSaveDataGroup(uiParent*,const Setup&);
    				~uiSaveDataGroup(){};

    bool 			getNamesToBeSaved(BufferStringSet&,
	    					  TypeSet<int>&);
    int				indexOf( const char* nm ) const
				{ return itmnames_.indexOf(nm); }
    const char*			itemName( int idx )
    				{ return itmnames_.get(idx); }
    void 			changeLogUIOutput(CallBacker*);

protected:

    ObjectSet<CtxtIOObj>       	ctio_;
    uiCheckBox*			checkallfld_;
    ObjectSet<uiGroup> 		objgrps_;
    ObjectSet<uiLabel> 		titlelblflds_;
    ObjectSet<uiLabel> 		lblflds_;
    ObjectSet<uiGenInput> 	nameflds_;
    ObjectSet<uiCheckBox> 	boxflds_;
    ObjectSet<uiIOObjSel>  	ioobjselflds_;
    const BufferStringSet	itmnames_;
    bool 			saveasioobj_;

    void			checkAll(CallBacker*);
};


mClass(uiWellAttrib) uiSaveDataDlg : public uiDialog
{
public: 
				uiSaveDataDlg(uiParent*,const Data&,
							const DataWriter&);
				~uiSaveDataDlg();

protected :

    ObjectSet<CtxtIOObj>      	wvltctioset_;
    ObjectSet<CtxtIOObj>      	seisctioset_;

    uiSaveDataGroup* 		savelogsfld_;
    uiSaveDataGroup* 		savewvltsfld_;
    uiGenInput* 		saveasfld_;
    uiLabeledSpinBox*		repeatfld_;
    const Data& 		data_;
    const DataWriter&		datawriter_;

    bool 			acceptOK(CallBacker*);
    void 			changeLogUIOutput(CallBacker*);
};

}; //namespace WellTie

#endif



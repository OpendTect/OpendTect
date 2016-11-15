#pragma once
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "wellmarker.h"
#include "uistring.h"
#include "wellextractdata.h"
#include "uiwellextractparams.h"

class DBKeySet;
class IOObj;
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiListBox;
class uiListBoxChoiceIO;


mExpClass(uiWell) uiMultiWellLogSel : public uiGroup
{ mODTextTranslationClass(uiMultiWellLogSel);
public:
   			uiMultiWellLogSel(uiParent*,const bool, 
					  const uiWellExtractParams::Setup* = 0,
					  const BufferStringSet* wellnms=0,
					  const BufferStringSet* lognms=0 );
			uiMultiWellLogSel(uiParent*, const bool,
					  const DBKey& singlewid,
					 const uiWellExtractParams::Setup* = 0);
			~uiMultiWellLogSel();

    void		selectOnlyWritableWells();

    void		getSelLogNames(BufferStringSet&) const;
    void		getSelWellNames(BufferStringSet&) const;
    void		getSelWellIDs(BufferStringSet&) const;
    void		getSelWellIDs(DBKeySet&) const;
    Well::ExtractParams* getWellExtractParams(); 


    void		setSelLogNames(const BufferStringSet&);
    void		setSelWellNames(const BufferStringSet&);
    void		setSelWellIDs(const BufferStringSet&);
    void		setSelWellIDs(const DBKeySet&);
    void		setWellExtractParams(const Well::ExtractParams&);

    void		update(); //call this when data changed
    bool		isWellExtractParamsUsed() const
			{ return wellextractparamsfld_; }
protected:

    ObjectSet<IOObj>	wellobjs_;

    const DBKey*	singlewid_;

    uiListBox*		wellsfld_;
    uiListBox*		logsfld_;
    uiListBoxChoiceIO*	wellschoiceio_;
    uiWellExtractParams* wellextractparamsfld_;
    const uiWellExtractParams::Setup*	setup_;

    void		init(const bool);
    void		getFromScreen(CallBacker*);

    void		readWellChoiceDone(CallBacker*);
    void		writeWellChoiceReq(CallBacker*);
    void		updateLogsFldCB(CallBacker*);
    bool		singlelog_;
};

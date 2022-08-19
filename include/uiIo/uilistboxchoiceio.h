#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "bufstringset.h"
#include "callback.h"
#include "multiid.h"
#include "uistring.h"

class uiListBox;
class CtxtIOObj;


/*!\brief Allows save/restore of chosen items for a uiListBox.

 If you provide keys (one for each item), then these will be stored too.

 If you have no keys, then you can simply do setChosen( chosenNames() ).
 Otherwise you need to construct the display names yourself and then use
 setChosen().

 Note: this object needs to be deleted explicitly; it will not be deleted
 automatically like the uiListBox itself.

 */

mExpClass(uiIo) uiListBoxChoiceIO : public CallBacker
{ mODTextTranslationClass(uiListBoxChoiceIO);
public:

			uiListBoxChoiceIO(uiListBox&,const char* omftypekey);
			~uiListBoxChoiceIO();

			// When store requested, fill with a key for each item:
    TypeSet<MultiID>&	keys()			{ return keys_; }

			// When read done:
    const BufferStringSet& chosenNames() const	{ return chosennames_; }
    const TypeSet<MultiID>& chosenKeys() const	{ return keys_; }
    void		setChosen(const BufferStringSet& names);

    Notifier<uiListBoxChoiceIO> storeRequested; //!< opportunity to set the keys
    Notifier<uiListBoxChoiceIO> readDone;	//!< now use setChosen

protected:

    uiListBox&		lb_;
    CtxtIOObj&		ctio_;
    TypeSet<MultiID>	keys_;
    BufferStringSet	chosennames_;

    void		readReqCB(CallBacker*);
    void		storeReqCB(CallBacker*);
};

#ifndef uilistboxchoiceio_h
#define uilistboxchoiceio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "bufstringset.h"
#include "callback.h"
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
{
public:

			uiListBoxChoiceIO(uiListBox&,const char* omftypekey);
			~uiListBoxChoiceIO();

			// When store requested, fill with a key for each item:
    BufferStringSet&	keys()			{ return keys_; }

			// When read done:
    const BufferStringSet& chosenNames() const	{ return chosennames_; }
    const BufferStringSet& chosenKeys() const	{ return keys_; }
    void		setChosen(const BufferStringSet& names);

    Notifier<uiListBoxChoiceIO> storeRequested; //!< opportunity to set the keys
    Notifier<uiListBoxChoiceIO> readDone;	//!< now use setChosen

protected:

    uiListBox&		lb_;
    CtxtIOObj&		ctio_;
    BufferStringSet	keys_;
    BufferStringSet	chosennames_;

    void		readReqCB(CallBacker*);
    void		storeReqCB(CallBacker*);

};



#endif

#ifndef uiiosel_h
#define uiiosel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiiosel.h,v 1.3 2001-04-30 14:59:05 bert Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class UserIDSet;
class uiLabeledComboBox;
class uiPushButton;
class IOPar;


/*! \brief UI element for selection of data objects

*/

class uiIOSelect : public uiGroup
{
public:

			uiIOSelect(uiObject*,const CallBack& do_selection,
				   const char* txt,
				   bool selection_editable=true,
				   bool withclear=false);
			~uiIOSelect();

    void		notifySelection( const CallBack& cb )
			{ seldonecb_ = cb; }

    const char*		getInput() const;
    const char*		getKey() const;
    void		setInput(const char* key);
			//!< Will fetch user name using userNameFromKey

    int			nrItems() const		 { return entries_.size(); }
    int			getCurrentItem() const;
    void		setCurrentItem(int);
    const char*		getItem( int idx ) const { return *entries_[idx]; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

protected:

    CallBack		doselcb_;
    CallBack		seldonecb_;
    ObjectSet<BufferString>	entries_;
    bool		withclear_;

    uiLabeledComboBox*	inp_;
    uiPushButton*	selbut_;

    void		doSel(CallBacker*);
    void		selDone(CallBacker*);
			//!< Subclass must call it - base class can't
			//!< determine whether a selection was successful.

    virtual const char*	userNameFromKey( const char* s ) const	{ return s; }
			//!< If 0 returned, then if possible,
			//!< that entry is not entered in the entries_.

    void		updateFromEntries();


};


class uiIOFileSelect : public uiIOSelect
{
public:
			uiIOFileSelect(uiObject*,const char* txt,
					bool for_read,
					const char* inp=0,
					bool withclear=false);

    void		setFilter( const char* f )	{ filter = f; }

protected:

    void		doFileSel(CallBacker*);
    bool		forread;
    BufferString	filter;

};


#endif

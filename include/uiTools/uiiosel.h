#ifndef uiiosel_h
#define uiiosel_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiiosel.h,v 1.2 2001-04-27 16:49:09 bert Exp $
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
    void		setInput(const char*);

    int			nrItems() const;
    int			getItem() const;
    const char*		getItemText(int) const;
    void		setCurrentItem(int);
    void		setItems(const UserIDSet&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    CallBack		doselcb_;
    CallBack		seldonecb_;
    UserIDSet&		inpsels_;
    bool		withclear_;

    uiLabeledComboBox*	inp_;
    uiPushButton*	selbut_;

    void		doSel(CallBacker*);
    void		selDone(CallBacker*);

    void		setCurrentFromIOPar(const IOPar &);

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

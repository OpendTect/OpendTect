#ifndef uiListBox_H
#define uiListBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.11 2001-05-30 21:49:32 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class i_QListBox;
class i_listMessenger;
class PtrUserIDObjectSet;


class uiListBox : public uiWrapObj<i_QListBox>
{
    friend class	i_listMessenger;
    friend class	i_QListBox;

public:

                        uiListBox(uiObject* parnt=0, 
				  const char* nm="uiListBox",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);
			uiListBox(uiObject*,const PtrUserIDObjectSet&,
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBox();

			/*! preferred number of lines. if set to 0, then 
                            it is determined by the number of items in list.
			    If set to 1, then the list can not 
			    grow/shrink vertically.
			*/
    void 		setLines( int prefNrLines )
			{ 
			    if(prefNrLines >= 0) nLines=prefNrLines;
			    setStretch( 1, isSingleLine() ? 0 : 1 );
			}

    void                notify( const CallBack& cb ) { notifyCBL += cb; }
			//!< Triggered when selection has changed. 

    int			size() const;
    bool		isPresent(const char*) const;
    bool		isSelected(int) const;
    void		setSelected(int,bool yn=true);
    void		selAll(bool yn=true);
    virtual void	clear()			{ setCurrentItem(0); }

    void		empty();
    void		addItem(const char*); 
    void		addItems(const char**); 
    void		addItems(const PtrUserIDObjectSet&);
    void		setItemText(int,const char*);
    int			currentItem() const;
    const char*		getText() const	 { return textOfItem(currentItem()); }
    const char*		textOfItem(int) const;
    void                setCurrentItem(int);
    void                setCurrentItem(const char*); //!< First match

    virtual uiSize	minimumSize() const; //!< \reimp
    virtual bool	isSingleLine() const { return nLines==1; }

protected:

    const QWidget*	qWidget_() const;

    virtual void        notifyHandler() //!< Handler called from Qt.
			{ Notifier(); }
    void                Notifier()     { notifyCBL.doCall(this); }
    CallBackList        notifyCBL;

    int 		fieldWdt;
    int 		nLines;
    BufferString	rettxt;

private:

    i_listMessenger&    _messenger;

};

#endif

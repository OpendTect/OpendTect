#ifndef uiListBox_H
#define uiListBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.4 2001-04-27 16:49:02 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class i_QListBox;
class i_listMessenger;
class UserIDSet;


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
			uiListBox(uiObject*,const UserIDSet&,
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
    void		clear();
    void		addItem(const char*); 
    void		addItems(const char**); 
    void		addItems(const UserIDSet&);

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

private:

    i_listMessenger&    _messenger;

};

#endif

#ifndef uiListBox_H
#define uiListBox_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.h,v 1.2 2001-01-26 09:54:07 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class i_QListBox;
class i_listMessenger;

class uiListBox : public uiWrapObj<i_QListBox>
{
friend class i_listMessenger;
friend class i_QListBox;
public:

                        uiListBox(  uiObject* parnt=0, 
				    const char* nm="uiListBox",
				    bool isMultiSelect = false,
				    int preferredNrLines = 0,
				    int preferredFieldWidth = 0 );

    virtual 		~uiListBox();

    virtual bool	isSingleLine() const { return nLines==1; }

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

    bool		isSelected ( int index ) const;
    int			insertItem( const char* text ); 
    int			insertItems( const char** textList ); 

    int getCurId()	{ return cur_id; }
    //!< \return current Id, which equals the number of items in the box - 1.
    // UNLESS items have been removed, but that's not supported.

    virtual uiSize	minimumSize() const; //!< \reimp

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

    int			cur_id;
    int			getNewId() { return ++cur_id; }
};

#endif // uiListBox_H

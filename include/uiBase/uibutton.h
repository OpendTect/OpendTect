#ifndef uibutton_H
#define uibutton_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.h,v 1.3 2001-02-16 17:01:37 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QButton;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QToolButton;
class i_ButMessenger;
class uiButtonGroup;
class QButtonGroup;
template <class T> class i_QButtWrapper;
template <class T> class i_QObjWrapper;

mTemplTypeDefT(i_QButtWrapper,QButton,i_QButton)
mTemplTypeDefT(i_QButtWrapper,QPushButton,i_PushButton)
mTemplTypeDefT(i_QButtWrapper,QRadioButton,i_RadioButton)
mTemplTypeDefT(i_QButtWrapper,QCheckBox,i_CheckBox)
mTemplTypeDefT(i_QButtWrapper,QToolButton,i_ToolButton)
mTemplTypeDefT(i_QObjWrapper,QButtonGroup,i_QButtonGroup)

//! Button Abstract Base class
class uiButton : public uiObject
{

    friend class	i_ButMessenger;

public:
			uiButton(uiObject* parnt,const char*);
    virtual		~uiButton()		{}

    virtual QButton&    qButton() = 0;
    inline const QButton& qButton() const
                        { return ((uiButton*)this)->qButton(); }

    virtual bool        isSingleLine() const { return true; }

    virtual void        notify( const CallBack& cb ) { notifyCBL += cb; }

    virtual void	setText(const char*);
    const char*		text();

protected:

    //! Button signals emitted by Qt.
    enum notifyTp	{ clicked, pressed, released, toggled, stateChanged };

    //! Handler called from Qt.
    virtual void        notifyHandler( notifyTp ) = 0; 

    void                Notifier()     { notifyCBL.doCall(this); }
    CallBackList        notifyCBL;

    bool		add2LM(uiObject*) const;

};

class uiPushButton : public uiMltplWrapObj<uiButton,i_PushButton>
{
public:
			uiPushButton(uiObject* parnt,const char* txt); 
    virtual		~uiPushButton();

    virtual QButton&    qButton();
    void		setDefault( bool yn = true);
protected:

    const QWidget*	qWidget_() const;
    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == clicked ) Notifier(); }
};


class uiRadioButton : public uiMltplWrapObj<uiButton,i_RadioButton>
{                        
public:
			uiRadioButton(uiObject* parnt,const char* txt);
    virtual             ~uiRadioButton();

    virtual QButton&    qButton(); 

    bool 		isChecked() const;
    virtual void 	setChecked( bool check );

protected:

    const QWidget*	qWidget_() const;
    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == clicked ) Notifier(); }
};


class uiCheckBox: public uiMltplWrapObj<uiButton,i_CheckBox>
{
public:

                        uiCheckBox(uiObject* parnt,const char* txt);
    virtual             ~uiCheckBox();

    virtual QButton&    qButton(); 

    bool 		isChecked () const;
    void 		setChecked ( bool check ) ;

protected:

    const QWidget*	qWidget_() const;
    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == stateChanged ) Notifier(); }
};


class uiToolButton : public uiMltplWrapObj<uiButton,i_ToolButton>
{
public:
                        uiToolButton(uiObject* parnt,const char* txt);
    virtual             ~uiToolButton();

    virtual QButton&    qButton(); 


    bool 		usesBigPixmap () const;
    bool 		usesTextLabel () const;

protected:

    const QWidget*	qWidget_() const;
    virtual void        notifyHandler( notifyTp tp ) 
			{ if ( tp == clicked ) Notifier(); }
};


class uiButtonGroup : public uiWrapObj<i_QButtonGroup>
{
public:
			uiButtonGroup(uiObject* parnt,const char* txt,
                                        bool vertical = true, int strips = 1 ); 
    virtual             ~uiButtonGroup();

    virtual QButtonGroup& qButtonGroup();

protected:

    const QWidget*	qWidget_() const;

};


#endif

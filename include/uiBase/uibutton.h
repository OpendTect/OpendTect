#ifndef uibutton_H
#define uibutton_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.h,v 1.8 2002-01-07 13:17:01 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class uiButtonGroup;
class uiButtonBody;

class uiPushButtonBody;
class uiRadioButtonBody;
class uiCheckBoxBody;
class uiToolButtonBody;
class ioPixmap;


//! Button Abstract Base class
class uiButton : public uiObject
{
public:
			uiButton( uiParent*, const char*, const CallBack*,
				  uiObjectBody&);

    virtual		~uiButton()		{}

    virtual void	setText(const char*);
    const char*		text();

    Notifier<uiButton>	activated;
};

class uiPushButton : public uiButton
{
public:
				uiPushButton( uiParent*, const char* nm,
					      const CallBack* cb =0); 
				uiPushButton( uiParent*, const char* nm,
					      const ioPixmap&,
					      const CallBack* cb =0); 

    void			setDefault( bool yn = true);

private:

    uiPushButtonBody*		body_;
    uiPushButtonBody&		mkbody(uiParent*,const ioPixmap*,const char*);

};


class uiRadioButton : public uiButton
{                        
public:
				uiRadioButton( uiParent*, const char* nm,
					       const CallBack* cb =0);

    bool			isChecked() const;
    virtual void		setChecked( bool check );

private:

    uiRadioButtonBody*		body_;
    uiRadioButtonBody&		mkbody(uiParent*, const char*);

};


class uiCheckBox: public uiButton
{
public:

				uiCheckBox( uiParent*, const char* nm,
					    const CallBack* cb =0);

    bool			isChecked () const;
    void			setChecked ( bool check ) ;

    virtual void		setText(const char*);

private:

    uiCheckBoxBody*		body_;
    uiCheckBoxBody&		mkbody(uiParent*, const char*);

};


class uiToolButton : public uiButton
{
public:
				uiToolButton( uiParent*, const char* nm,
                                              const CallBack* cb =0);
				uiToolButton( uiParent*, const char* nm,
					      const ioPixmap&,
					      const CallBack* cb =0); 
private:

    uiToolButtonBody*		body_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap*, const char*); 

};


//! Button Abstract Base class
class uiButtonBody
{
    friend class        i_ButMessenger;

public:
			uiButtonBody()				{}
    virtual		~uiButtonBody()				{}

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled, stateChanged };

protected:
    //! Handler called from Qt.
    virtual void        notifyHandler( notifyTp )		=0;
};




#endif

#ifndef uigeninput_impl_h
#define uigeninput_impl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink / Bert
 Date:          21/2/2002 / Sep 2011
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uispinbox.h"
#include "userinputobj.h"

class uiCheckBox;
class uiRadioButton;

/*! \brief Generalised data input field.

  Provides a generalized interface towards data inputs from the user interface.

  Of course it doesn't make much sense to use f.e. setBoolValue on an element
  that is supposed to input double precision float's, but that's up to the
  programmer to decide.

*/

class uiGenInputInputFld : public CallBacker
{
public:
			uiGenInputInputFld(uiGenInput*,const DataInpSpec&) ;
    virtual		~uiGenInputInputFld();
    virtual int		nElems() const			{ return 1; }

    virtual UserInputObj* element( int idx=0 )		= 0;
    const UserInputObj* element(int idx=0) const;
    virtual uiObject*	mainObj()			= 0;
    virtual uiObject*	elemObj(int idx=0);
    const uiObject*	elemObj(int idx=0) const;

    virtual bool	isUndef(int) const;
    const char*		text(int) const;
    int			getIntValue(int) const;
    od_int64		getInt64Value(int) const;
    float		getFValue(int) const;
    double		getDValue(int) const;
    bool		getBoolValue(int) const;

    template <class T>
    void		setValue( T t, int idx )
			{
			    UserInputObj* obj = element( idx );
			    if ( !obj )
			    {
				mDynamicCastGet(uiSpinBox*,sb,elemObj(idx))
				if ( sb ) sb->setValue( t );
				return;
			    }

			    if ( mIsUdf(t) )
				obj->setEmpty();
			    else
				obj->setValue(t);
			}

    virtual void	setText(const char*,int);
    void		setValue(bool,int);

    void		setEmpty();
    void		display(bool,int elemidx);
    bool		isReadOnly(int idx=0) const;
    virtual void	setReadOnly(bool yn=true,int idx=-1);
    void		setSensitive(bool yn,int elemidx=-1);

    DataInpSpec&	spec()				{ return spec_; }
    const DataInpSpec&	spec() const			{ return spec_; }

    bool		update(const DataInpSpec&);
    virtual void	updateSpec();

    void		valChangingNotify(CallBacker*);
    void		valChangedNotify(CallBacker*);
    void		updateReqNotify(CallBacker*);

protected:

    DataInpSpec&	spec_;
    uiGenInput*		p_;

    virtual bool	update_(const DataInpSpec&);
    void		init();

};


class uiGenInputBoolFld : public UserInputObjImpl<bool>, public uiGroup
{ mODTextTranslationClass(UserInputObjImpl)
public:

			uiGenInputBoolFld(uiParent*,
				    const uiString& truetext=uiStrings::
								sEmptyString(),
				    const uiString& falsetext=uiStrings::
								sEmptyString(),
				    bool initval = true,
				    const char* nm="Bool Input");

			uiGenInputBoolFld(uiParent*,
				    const DataInpSpec& spec,
				    const char* nm="Bool Input");

    virtual const char*	text() const;
    virtual void        setText(const char* t);

    bool		notifyValueChanged( const CallBack& cb )
			    { valueChanged.notify(cb); return true; }

    Notifier<uiGenInputBoolFld> valueChanged;

    virtual void	setReadOnly(bool ro=true);
    virtual bool	isReadOnly() const;

    virtual bool	update_(const DataInpSpec& spec);

    virtual void	setToolTip(const uiString&);

protected:

    void		init(uiParent*,const uiString&,const uiString&,bool);

    virtual bool	getvalue_() const			{ return yn_; }
    virtual void	setvalue_( bool );

    virtual bool	notifyValueChanging_( const CallBack& )	{ return false;}
    virtual bool	notifyValueChanged_( const CallBack& cb )
			    { valueChanged.notify(cb); return true; }
    virtual bool	notifyUpdateRequested_(const CallBack&) { return false;}

    void		selected(CallBacker*);

    virtual uiObject*	mainobject();

    uiString		truetxt_;
    uiString		falsetxt_;
    bool		yn_;
    uiObject*		butgrp_;
    uiCheckBox*		checkbox_;
    uiRadioButton*	rb1_;
    uiRadioButton*	rb2_;
};


class uiGenInputIntFld : public UserInputObjImpl<int>, public uiSpinBox
{
public:
			uiGenInputIntFld(uiParent*,int val=0,
					 const char* nm="Int Input");
			uiGenInputIntFld(uiParent*,const DataInpSpec&,
					 const char* nm="Int Input");
    virtual void	setReadOnly(bool);
    virtual bool	isReadOnly() const;

    virtual bool	update_(const DataInpSpec&);
    virtual void	setToolTip(const uiString&);

protected:

    virtual int		getvalue_() const;
    virtual void	setvalue_(int);

    virtual bool	notifyValueChanging_(const CallBack&);
    virtual bool	notifyValueChanged_(const CallBack&);
    virtual bool	notifyUpdateRequested_(const CallBack&);
};


class uiGenInputInt64Fld : public UserInputObjImpl<od_int64>, public uiSpinBox
{
public:
			uiGenInputInt64Fld(uiParent*,od_int64 val=0,
					 const char* nm="Int Input");
			uiGenInputInt64Fld(uiParent*,const DataInpSpec&,
					 const char* nm="Int Input");
    virtual void	setReadOnly(bool);
    virtual bool	isReadOnly() const;

    virtual bool	update_(const DataInpSpec&);
    virtual void	setToolTip(const uiString&);

protected:

    virtual od_int64	getvalue_() const;
    virtual void	setvalue_(od_int64);

    virtual bool	notifyValueChanging_(const CallBack&);
    virtual bool	notifyValueChanged_(const CallBack&);
    virtual bool	notifyUpdateRequested_(const CallBack&);
};

#endif

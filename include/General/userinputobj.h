#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/2/2002
________________________________________________________________________

-*/

#include "generalmod.h"
#include "datainpspec.h"
#include "convert.h"
#include "undefval.h"

class CallBack;
class BufferStringSet;
class uiString;


mExpClass(General) UserInputObj
{
public:

			UserInputObj()			{}
    virtual		~UserInputObj()			{}

    virtual const char* text() const			= 0;
    virtual int		getIntValue() const		= 0;
    virtual od_int64	getInt64Value() const		= 0;
    virtual double	getDValue() const		= 0;
    virtual float	getFValue() const		= 0;
    virtual bool	getBoolValue() const		= 0;

    virtual void	setText(const char*)		= 0;
    virtual void	setValue(const char* s);
    virtual void	setValue(int)			= 0;
    virtual void	setValue(od_int64)		= 0;
    virtual void	setValue(double)		= 0;
    virtual void	setValue(float)			= 0;
    virtual void	setValue(bool)			= 0;

    virtual void	setReadOnly( bool = true )	= 0;
    virtual bool	isReadOnly() const		= 0;

    virtual void	setEmpty()			= 0;
    virtual void	addItem(const uiString&);

			/*! \brief intermediate value available
			    \return true if this notification is supported */
    bool		notifyValueChanging( const CallBack& cb );

			/*! \brief value change complete cq. commited
			    \return true if this notification is supported */
    bool		notifyValueChanged( const CallBack& cb );

			/*! \return true if this notification is supported */
    bool		notifyUpdateRequested( const CallBack& cb );

			//! return false if not updated for whatever reason.
    bool		update( const DataInpSpec& s );

    virtual void	setToolTip(const uiString&)		= 0;

protected:

			//! return false if not available
    virtual bool	notifyValueChanging_(const CallBack&)	=0;
			//! return false if not available
    virtual bool	notifyValueChanged_(const CallBack&)	=0;
    virtual bool	notifyUpdateRequested_(const CallBack&)	=0;
    virtual bool	update_( const DataInpSpec&)		=0;

public:
    mDeprecated		("Use getDValue")
    double		getdValue() const	{ return getDValue(); }
    mDeprecated		("Use getFValue")
    float		getfValue() const	{ return getFValue(); }
};


template<class T>
mClass(General) UserInputObjImpl : public UserInputObj
{
public:

    const char* text() const override
			{ return Conv::to<const char*>( getvalue_() ); }
    int		getIntValue() const override
			    { return Conv::to<int>( getvalue_() ); }
    od_int64	getInt64Value() const override
			    { return Conv::to<od_int64>( getvalue_() ); }
    double	getDValue() const override
			    { return Conv::to<double>( getvalue_() );}
    float	getFValue() const override
			    { return Conv::to<float>( getvalue_() ); }
    bool	getBoolValue() const override
			    { return Conv::to<bool>( getvalue_() ); }

    void	setText( const char* s ) override
			    { setvalue_( Conv::to<T>(s) ); }
    void	setValue( const char* s ) override
			    { setText( s ); }
    void	setValue( int i ) override
			    { setvalue_( Conv::to<T>(i) ); }
    void	setValue( od_int64 i ) override
			    { setvalue_( Conv::to<T>(i) ); }
    void	setValue( double d ) override
			    { setvalue_( Conv::to<T>(d) ); }
    void	setValue( float f ) override
			    { setvalue_( Conv::to<T>(f) ); }
    void	setValue( bool b ) override
			    { setvalue_( Conv::to<T>(b) ); }

    void	setEmpty() override
			    { setvalue_(mUdf(T)); }

protected:
			// return true if implemented.
    virtual bool	setEmpty_()			{ return false; }

    virtual void	setvalue_( T v )		= 0;
    virtual T		getvalue_() const		= 0;

};


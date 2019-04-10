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

};


template<class T>
mClass(General) UserInputObjImpl : public UserInputObj
{
public:

    virtual const char*	text() const
			{ return Conv::to<const char*>( getvalue_() ); }
    virtual int		getIntValue() const
			    { return Conv::to<int>( getvalue_() ); }
    virtual od_int64	getInt64Value() const
			    { return Conv::to<od_int64>( getvalue_() ); }
    virtual double	getDValue() const
			    { return Conv::to<double>( getvalue_() );}
    virtual float	getFValue() const
			    { return Conv::to<float>( getvalue_() ); }
    virtual bool	getBoolValue() const
			    { return Conv::to<bool>( getvalue_() ); }

    virtual void	setText( const char* s )
			    { setvalue_( Conv::to<T>(s) ); }
    virtual void	setValue( const char* s )
			    { setText( s ); }
    virtual void	setValue( int i )
			    { setvalue_( Conv::to<T>(i) ); }
    virtual void	setValue( od_int64 i )
			    { setvalue_( Conv::to<T>(i) ); }
    virtual void	setValue( double d )
			    { setvalue_( Conv::to<T>(d) ); }
    virtual void	setValue( float f )
			    { setvalue_( Conv::to<T>(f) ); }
    virtual void	setValue( bool b )
			    { setvalue_( Conv::to<T>(b) ); }

    void		setEmpty()
			    { setvalue_(mUdf(T)); }

protected:
			// return true if implemented.
    virtual bool	setEmpty_()			{ return false; }

    virtual void	setvalue_( T v )		= 0;
    virtual T		getvalue_() const		= 0;

};

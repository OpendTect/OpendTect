#ifndef userinputobj_h
#define userinputobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/2/2002
 RCS:           $Id: userinputobj.h,v 1.2 2002-03-12 12:11:40 arend Exp $
________________________________________________________________________

-*/

#include <basictypes.h>

class CallBack;
class uiObject;
class DataInpSpec;

class UserInputObj
{
public:

                        UserInputObj()			{}

    virtual const char* text() const			= 0;
    virtual int         getIntValue()  const		= 0;
    virtual double      getValue()     const		= 0;
    virtual float       getfValue()    const		= 0;
    virtual bool        getBoolValue() const		= 0;

    virtual void        setText( const char* )		= 0;
    virtual void        setValue( int i )		= 0;
    virtual void        setValue( double d )		= 0;
    virtual void        setValue( float f )		= 0;
    virtual void        setValue( bool b )		= 0;

    virtual void	setReadOnly( bool = true )	= 0;
    virtual bool	isReadOnly() const		= 0;

			//! sets current value as clear value
    virtual void	initClearValue()		= 0;
    virtual void	clear()				= 0;

		        /*! \brief intermediate value available
			    \return true if this notification is supported
			*/
    bool		notifyValueChanging( const CallBack& cb )
			    { return notifyValueChanging_( cb ); }

		        /*! \brief value change complete cq. commited
			    \return true if this notification is supported
			*/
    bool		notifyValueChanged( const CallBack& cb )
			    { return notifyValueChanged_( cb ); }


//TODO : replace pure virtual functions with default impl....
#define DEBUG_VIRTUAL
#ifdef DEBUG_VIRTUAL
    virtual bool	update( const DataInpSpec& )		=0;

protected:

    virtual bool	notifyValueChanging_( const CallBack& )	=0;
    virtual bool	notifyValueChanged_( const CallBack& )	=0;

#else
			//! returns true if successful
    virtual bool	update( const DataInpSpec& )		{ return false;}

protected:

    virtual bool	notifyValueChanging_( const CallBack& )	{ return false;}
    virtual bool	notifyValueChanged_( const CallBack& )	{ return false;}

#endif
};


template<class T>
class UserInputObjImpl : public UserInputObj
{
public:
                        UserInputObjImpl()
			    : clearvalset_(false)	{}

    virtual const char*	text() const
			    { return convertTo<const char*>( getvalue_() ); }
    virtual int		getIntValue() const
			    { return convertTo<int>( getvalue_() ); }
    virtual double	getValue() const
			    { return convertTo<double>( getvalue_() ); }
    virtual float	getfValue() const
			    { return convertTo<float>( getvalue_() ); }
    virtual bool	getBoolValue() const
			    { return convertTo<bool>( getvalue_() ); }

    virtual void	setValue( int i )
			    { setvalue_( convertTo<T>(i) ); }
    virtual void	setText( const char* s )
			    { setvalue_( convertTo<T>(s) ); }
    virtual void	setValue( double d )
			    { setvalue_( convertTo<T>(d) ); }
    virtual void	setValue( float f )
			    { setvalue_( convertTo<T>(f) ); }
    virtual void	setValue( bool b )
			    { setvalue_( convertTo<T>(b) ); }

    void		initClearValue()
			    { setClearValue( getvalue_() ); }
    void		clear()
			    {
				if ( clearvalset_ ) setvalue_(clearval_);
				else clear_();
			    }
    void		setClearValue( const T& v )
			    { clearvalset_ = true; clearval_ = v; }

protected:

    T			clearval_;
    bool		clearvalset_;

    virtual void	clear_()			{}
    virtual void	setvalue_( T v )		= 0;
    virtual T		getvalue_() const		= 0;

};


#endif

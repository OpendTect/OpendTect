#ifndef userinputobj_h
#define userinputobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/2/2002
 RCS:           $Id: userinputobj.h,v 1.9 2005-01-12 16:13:43 arend Exp $
________________________________________________________________________

-*/

#include "basictypes.h"
#include "datainpspec.h"

class CallBack;
class PtrUserIDObjectSet;
class BufferString;
class BufferStringSet;
template <class T> class ObjectSet;

class UserInputObj
{
public:

                        UserInputObj()			{}
    virtual		~UserInputObj()			{}

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

    virtual bool	hasItems()			{ return false; }
    virtual void        addItem( const char* txt )	{ setText( txt ); }
    void		addItems(const char**);
    void		addItems(const BufferStringSet&);


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


			//! return false if not updated for whatever reason.
    bool		update( const DataInpSpec& s )		
			    { return update_(s); }

protected:
			//! return false if not available
    virtual bool	notifyValueChanging_( const CallBack& )	=0;

			//! return false if not available
    virtual bool	notifyValueChanged_( const CallBack& )	=0;
    virtual bool	update_( const DataInpSpec&)		=0;
};


template<class T>
class UserInputObjImpl : public UserInputObj
{
public:
                        UserInputObjImpl()
			    : clearvalset_(false)	{}

    virtual const char*	text() const
			{ return conv2<const char*>( getvalue_() ); }
    virtual int		getIntValue() const
			    { return conv2<int>( getvalue_() ); }
    virtual double	getValue() const
			    { return conv2<double>( getvalue_() );}
    virtual float	getfValue() const
			    { return conv2<float>( getvalue_() ); }
    virtual bool	getBoolValue() const
			    { return conv2<bool>( getvalue_() ); }

    virtual void	setValue( int i )
			    { setvalue_( conv2<T>(i) ); }
    virtual void	setText( const char* s )
			    { setvalue_( conv2<T>(s) ); }
    virtual void	setValue( double d )
			    { setvalue_( conv2<T>(d) ); }
    virtual void	setValue( float f )
			    { setvalue_( conv2<T>(f) ); }
    virtual void	setValue( bool b )
			    { setvalue_( conv2<T>(b) ); }

    void		initClearValue()
			    { setClearValue( getvalue_() ); }
    void		clear()
			    {
				if ( clearvalset_ )
				    setvalue_(clearval_);
				else if ( !clear_() )
				    setvalue_( UndefValues<T>::initVal() );
			    }
    void		setClearValue( const T& v )
			    { clearvalset_ = true; clearval_ = v; }

protected:

    T			clearval_;
    bool		clearvalset_;

			// return true if implemented.
    virtual bool	clear_()			{ return false; }

    virtual void	setvalue_( T v )		= 0;
    virtual T		getvalue_() const		= 0;

};


#endif

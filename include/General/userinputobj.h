#ifndef userinputobj_h
#define userinputobj_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/2/2002
 RCS:           $Id: userinputobj.h,v 1.7 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include <basictypes.h>

class CallBack;
class DataInpSpec;
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
			    { update_pref(s); return update_(s); }

protected:
			//! return false if not available
    virtual bool	notifyValueChanging_( const CallBack& )	=0;

			//! return false if not available
    virtual bool	notifyValueChanged_( const CallBack& )	=0;
    virtual bool	update_( const DataInpSpec&)		=0;
    virtual void	update_pref( const DataInpSpec&)	=0;
};


template<class T>
class UserInputObjImpl : public UserInputObj
{
public:
                        UserInputObjImpl( bool prefempty = true )
			    : clearvalset_(false)
			    , prefempty_(prefempty)	{}

    virtual const char*	text() const
			{ return convertTo<const char*>(getvalue_(),
							      prefempty_); }
    virtual int		getIntValue() const
			    { return convertTo<int>(getvalue_(),prefempty_); }
    virtual double	getValue() const
			    { return convertTo<double>(getvalue_(),prefempty_);}
    virtual float	getfValue() const
			    { return convertTo<float>(getvalue_(),prefempty_); }
    virtual bool	getBoolValue() const
			    { return convertTo<bool>(getvalue_(),prefempty_); }

    virtual void	setValue( int i )
			    { setvalue_( convertTo<T>(i,prefempty_) ); }
    virtual void	setText( const char* s )
			    { setvalue_( convertTo<T>(s,prefempty_) ); }
    virtual void	setValue( double d )
			    { setvalue_( convertTo<T>(d,prefempty_) ); }
    virtual void	setValue( float f )
			    { setvalue_( convertTo<T>(f,prefempty_) ); }
    virtual void	setValue( bool b )
			    { setvalue_( convertTo<T>(b,prefempty_) ); }

    void		initClearValue()
			    { setClearValue( getvalue_() ); }
    void		clear()
			    {
				if ( clearvalset_ ) setvalue_(clearval_);
				else setvalue_( UndefValues<T>::emptyVal() );
			    }
    void		setClearValue( const T& v )
			    { clearvalset_ = true; clearval_ = v; }

    bool		preferEmpty() const		{ return prefempty_; }
    void		setPrefEmpty( bool yn=true )	{ prefempty_ = yn; }

protected:

    T			clearval_;
    bool		clearvalset_;
    bool		prefempty_;

    virtual void	clear_()			{}
    
    virtual void	setvalue_( T v )		= 0;
    virtual T		getvalue_() const		= 0;

    virtual void	update_pref( const DataInpSpec& s)
			    { prefempty_=s.preferEmpty(); }
};


#endif

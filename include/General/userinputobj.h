#ifndef userinputobj_h
#define userinputobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/2/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "datainpspec.h"
#include "convert.h"
#include "initval.h"

class CallBack;
class BufferString;
class BufferStringSet;
template <class T> class ObjectSet;

mExpClass(General) UserInputObj
{
public:

                        UserInputObj()			{}
    virtual		~UserInputObj()			{}

    virtual const char* text() const			= 0;
    virtual int         getIntValue()  const		= 0;
    virtual double      getdValue()     const		= 0;
    virtual float       getfValue()    const		= 0;
    virtual bool        getBoolValue() const		= 0;

    virtual void        setText(const char*)		= 0;
    virtual void        setValue(const char* s);
    virtual void        setValue(int)			= 0;
    virtual void        setValue(double)		= 0;
    virtual void        setValue(float )		= 0;
    virtual void        setValue(bool)			= 0;

    virtual void	setReadOnly( bool = true )	= 0;
    virtual bool	isReadOnly() const		= 0;

			//! sets current value as clear value
    virtual void	initClearValue()		= 0;
    virtual void	clear()				= 0;

    virtual void        addItem( const char* txt );
//    void		addItems(const char**);
//    void		addItems(const BufferStringSet&);


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

    virtual void        setToolTip(const char*)			= 0;

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
                        UserInputObjImpl()
			    : clearvalset_(false)	{}

    virtual const char*	text() const
			{ return Conv::to<const char*>( getvalue_() ); }
    virtual int		getIntValue() const
			    { return Conv::to<int>( getvalue_() ); }
    virtual double	getdValue() const
			    { return Conv::to<double>( getvalue_() );}
    virtual float	getfValue() const
			    { return Conv::to<float>( getvalue_() ); }
    virtual bool	getBoolValue() const
			    { return Conv::to<bool>( getvalue_() ); }

    virtual void	setText( const char* s )
			    { setvalue_( Conv::to<T>(s) ); }
    virtual void	setValue( const char* s )
			    { UserInputObj::setValue( s ); }
    virtual void	setValue( int i )
			    { setvalue_( Conv::to<T>(i) ); }
    virtual void	setValue( double d )
			    { setvalue_( Conv::to<T>(d) ); }
    virtual void	setValue( float f )
			    { setvalue_( Conv::to<T>(f) ); }
    virtual void	setValue( bool b )
			    { setvalue_( Conv::to<T>(b) ); }

    void		initClearValue()
			    { setClearValue( getvalue_() ); }
    void		clear()
			    {
				if ( clearvalset_ )
				    setvalue_(clearval_);
				else if ( !clear_() )
				    setvalue_( Values::initVal<T>() );
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


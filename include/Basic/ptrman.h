#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.10 2005-02-03 09:11:32 kristofer Exp $
________________________________________________________________________

-*/

#include <general.h>


#define mDefPtrMan1(Clss,PostSet, EraseFunc) \
\
template<class T> \
class Clss \
{ \
public: \
\
				Clss( T* p=0 ) : ptr_( 0 )	{ set(p); } \
				~Clss()		{ erase(); } \
    inline Clss<T>&		operator=( T* p ) \
				{ set(p); return *this; } \
\
    inline T*			ptr()			{ return ptr_; } \
    inline			operator T*()		{ return ptr_; } \
    inline			operator const T*() const { return ptr_; } \
    inline T*			operator ->()		{ return ptr_; } \
    inline const T*		operator ->() const	{ return ptr_; } \
    inline T&			operator *()		{ return *ptr_; } \

#define mDefPtrMan2(Clss,PostSet, EraseFunc) \
    inline bool			operator ==( const Clss& p ) const \
				{ return ptr_ == p.ptr_; } \
    inline bool			operator ==( const T* p ) const \
				{ return ptr_ == p; } \
    inline bool			operator !=( const Clss& p ) const \
				{ return ptr_ != p.ptr_; } \
    inline bool			operator !=( const T* p ) const \
				{ return ptr_ != p; } \
\
    inline bool			operator !() const { return !ptr_; } \
\
    void			erase() \
				{ EraseFunc; ptr_ = 0; } \
    void			set( T* p ) \
				{ erase(); ptr_=p; PostSet; }

#define mDefPtrMan3(Clss,PostSet, EraseFunc) \
private: \
\
    T*				ptr_; \
\
    Clss<T>&			operator=(const T& p) const; \
\
};


/*!\brief a simple autopointer.

It is assigned to a pointer, and takes over
the responsibility for its deletion.
For Arrays, use the ArrPtrMan class.

*/
mDefPtrMan1(PtrMan, , delete ptr_ )
mDefPtrMan2(PtrMan, , delete ptr_ )
mDefPtrMan3(PtrMan, , delete ptr_ )

/*!\brief a simple autopointer for arrays.

For Non-arrays, use the PtrMan class.

*/
mDefPtrMan1(ArrPtrMan, , delete [] ptr_)
mDefPtrMan2(ArrPtrMan, , delete [] ptr_)
mDefPtrMan3(ArrPtrMan, , delete [] ptr_)


#endif

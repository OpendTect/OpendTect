#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.5 2001-05-07 11:38:32 bert Exp $
________________________________________________________________________

-*/

#include <general.h>


#define mDefPtrMan1(Clss,ArrBrIfNec) \
\
template<class T> \
class Clss \
{ \
public: \
\
				Clss( T* ptr=0 ) : ptr_( ptr )	{} \
				~Clss()		{ erase(); } \
    inline Clss<T>&		operator=( T* ptr ) \
				{ erase(); ptr_ = ptr; return *this; } \
\
    inline			operator T*()	{ return ptr_; } \
    inline T*			operator ->()	{ return ptr_; } \
    inline T&			operator *()	{ return *ptr_; } \

#define mDefPtrMan2(Clss,ArrBrIfNec) \
    inline bool			operator ==( const Clss& p ) const \
				{ return ptr_ == p.ptr_; } \
    inline bool			operator ==( const T* ptr ) const \
				{ return ptr_ == ptr; } \
    inline bool			operator !=( const Clss& p ) const \
				{ return ptr_ != p.ptr_; } \
    inline bool			operator !=( const T* ptr ) const \
				{ return ptr_ != ptr; } \
\
    inline bool			operator !() const { return !ptr_; } \

#define mDefPtrMan3(Clss,ArrBrIfNec) \
private: \
\
    T*				ptr_; \
\
    Clss<T>&			operator=(const T& p) const; \
\
    void			erase() \
				{ delete ArrBrIfNec ptr_; ptr_ = 0; } \
\
};


/*!\brief a simple autopointer.

It is assigned to a pointer, and takes over
the responsibility for its deletion.
For Arrays, use the ArrPtrMan class.

*/
mDefPtrMan1(PtrMan,)
mDefPtrMan2(PtrMan,)
mDefPtrMan3(PtrMan,)

/*!\brief a simple autopointer for arrays.

For Non-arrays, use the PtrMan class.

*/
mDefPtrMan1(ArrPtrMan,[])
mDefPtrMan2(ArrPtrMan,[])
mDefPtrMan3(ArrPtrMan,[])


#endif

#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.1 2000-03-22 13:40:51 bert Exp $
________________________________________________________________________

PtrMan is a simple autopointer. It is assigned to a pointer, and takes over
the responsibility for its deletion.

*/

#include <general.h>

template<class T>
class PtrMan
{
public:
				PtrMan( T* ptr_ = 0 )
				: ptr( ptr_ )
				{}

				~PtrMan()
				{ erase(); }

    const T*			getPtr() const { return ptr; }

    const T*			operator=( T* ptr_ )
				{ erase(); ptr = ptr_; return ptr; }


private:
    void			erase()
				{ delete ptr; ptr = 0; }

    T*				ptr;
};

#endif

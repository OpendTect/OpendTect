#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.2 2000-11-21 13:37:21 bert Exp $
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

    T*				getPtr() const { return ptr; }

    T*				operator=( T* ptr_ )
				{ erase(); ptr = ptr_; return ptr; }


private:
    void			erase()
				{ delete ptr; ptr = 0; }

    T*				ptr;
};

#endif

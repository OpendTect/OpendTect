#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: ptrman.h,v 1.3 2001-02-13 17:15:46 bert Exp $
________________________________________________________________________

-*/

#include <general.h>


/*!\brief a simple autopointer.

It is assigned to a pointer, and takes over
the responsibility for its deletion.

*/

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

#ifndef refcount_h
#define refcount_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		13-11-2003
 Contents:	Basic functionality for reference counting
 RCS:		$Id: refcount.h,v 1.1 2003-11-24 08:31:14 kristofer Exp $
________________________________________________________________________

-*/

/*!
Macros to set up reference counting in a class. A refcount class is set up
by:
\code
class A
{
    mRefCountImpl(A);
public:
    //Your class stuff
};
\endcode

The macro will define a protected destructor, so you have to implement one
(even if it's a dummy {}). In your constructor, you have to put the mRefCountConstructor macro:
\code
A::A(params)
{
    mRefCountConstructor;

    //Your own stuff
}
\endcode

*/

#define mRefCountConstructor \
__refcount = 0

#define mRefCountImpl(ClassName) \
public: \
    void	ref() const \
		{ \
		    ClassName* obj = const_cast<ClassName*>(this); \
		    obj->__refcount++; \
		} \
    void	unRef() const \
		{ \
		    ClassName* obj = const_cast<ClassName*>(this); \
		    if ( !--obj->__refcount ) \
			delete obj; \
		} \
 \
    void	unRefNoDelete() const \
		{ \
		    ClassName* obj = const_cast<ClassName*>(this); \
		    obj->__refcount--; \
		} \
private: \
    int		__refcount;	\
protected: \
    		~ClassName();	\
private:

    
#endif

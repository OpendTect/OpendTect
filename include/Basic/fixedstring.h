#ifndef fixedstring_h
#define fixedstring_h
/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer
 Date:		April 2009
 RCS:		$Id: fixedstring.h,v 1.1 2009-05-05 01:10:14 cvskris Exp $
________________________________________________________________________

*/


/*! Class that holds a text string, and provides basic services around it. The string is assumed to be owned by someone else or be static. In any case, it is
assumed be be alive and well for the lifetime of the FixedString. */

mClass FixedString
{
public:
		FixedString(const char* p) : ptr_(p) {}
    FixedString& operator=(const FixedString& f) { return *this = f.ptr_; }
    FixedString& operator=(const char* p)	{ ptr_ = p; return *this; }

    bool	operator==(const char*) const;
    bool	operator!=(const char* s) const		{ return !(*this==s); }
    bool	operator==(const FixedString& f) const	{ return *this==f.ptr_;}
    bool	operator!=(const FixedString& f) const	{ return *this!=f.ptr_;}

    bool	isEmpty() const { return !ptr_ || !*ptr_; }
    int		size() const;

		operator const char*() const    	{ return ptr_; }

protected:

    const char*	ptr_;
};


inline bool operator==(const char* a, const FixedString& b)
{ return b==a; }


inline bool operator!=(const char* a, const FixedString& b)
{ return b!=a; }


#endif

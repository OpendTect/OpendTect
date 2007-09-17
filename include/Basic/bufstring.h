#ifndef bufstring_H
#define bufstring_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
 RCS:		$Id: bufstring.h,v 1.25 2007-09-17 15:25:48 cvskris Exp $
________________________________________________________________________

-*/

#include "commondefs.h"
#include <iosfwd>

/*!\brief String with variable length but guaranteed minimum buffer size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 4 extra bytes for variable length and 4 bytes for minimum length.

*/

class BufferString
{
public:
   			BufferString(const char* s=0,
				     unsigned int ml=mMaxUserIDLength);
   			BufferString(int i,unsigned int ml=mMaxUserIDLength);
   			BufferString(double d,unsigned int ml=mMaxUserIDLength);
   			BufferString(float f,unsigned int ml=mMaxUserIDLength);
			BufferString(const BufferString& bs);
   virtual		~BufferString();
   BufferString&	operator=(const BufferString& bs);
   BufferString&	operator=(int);
   BufferString&	operator=(double);
   BufferString&	operator=(float);
   BufferString&	operator+=(int);
   BufferString&	operator+=(double);
   BufferString&	operator+=(float);
   void			insertAt(int idx, const char*);
   			/*<If idx is after the current string's end, spaces
			   will be added before insert. */
			operator const char*() const;	
   char*		buf();
   const char*		buf() const;
   char&		operator [](int);
   const char&		operator [](int) const;
   char&		lastChar();
   const char&		lastChar() const;
   bool			isEmpty() const;
   void			setEmpty();
   unsigned int		size() const;
   unsigned int		bufSize() const;
   void			setBufSize(unsigned int);
   bool			operator==(const BufferString&) const;
   bool			operator!=(const BufferString&) const;
   bool			operator!=(const char*) const;
   bool			operator >(const char*) const;
   bool			operator <(const char*) const;

   BufferString&	operator=(const char*);
   BufferString&	operator+=(const char*);
   bool			operator==(const char*) const;

   static const BufferString& empty();

protected:

    char*		buf_;
    unsigned int	len_;
    const unsigned int	minlen_;

private:

    void		init();

};

std::ostream& operator <<(std::ostream&,const BufferString&);
std::istream& operator >>(std::istream&,BufferString&);



#endif

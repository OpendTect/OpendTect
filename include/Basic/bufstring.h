#ifndef bufstring_H
#define bufstring_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-4-2000
 Contents:	Variable buffer length strings with minimum size.
 RCS:		$Id: bufstring.h,v 1.28 2007-09-27 09:07:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "commondefs.h"
#include <iosfwd>

/*!\brief String with variable length but guaranteed minimum buffer size.

The minimum buffer size makes life easier in worlds where strcpy etc. rule.
Overhead is 4 extra bytes for variable length and 4 bytes for minimum length.

Passing a (char*) null pointer is no problem.

*/

class BufferString
{
public:
   			BufferString();
   			BufferString(const char* s);
   			BufferString(int i);
   			BufferString(double d);
   			BufferString(float f);
			BufferString(const BufferString& bs);
   			BufferString(const char*,const char*,const char* s3=0);
   			BufferString(const char*,int,const char* s3=0);
   			BufferString(const char*,double,const char* s3=0);
   			BufferString(const char*,float,const char* s3=0);
   virtual		~BufferString();
   inline bool		operator==( const BufferString& b ) const;
   inline bool		operator!=( const BufferString& b ) const;
   bool			operator==(const char*) const;
   inline bool		operator!=( const char* b ) const;

   inline char*		buf()				{ return buf_; }
   inline const char*	buf() const			{ return buf_; }
   inline		operator const char*() const	{ return buf_; }	
   inline char&		operator []( int idx )		{ return buf_[idx]; }
   const char&		operator []( int idx ) const	{ return buf_[idx]; }
   inline bool		isEmpty() const			{ return !(*buf_); }
   void			setEmpty();

   BufferString&	operator=(const char*);
   BufferString&	operator=(const BufferString& bs);
   BufferString&	operator=(int);
   BufferString&	operator=(float);
   BufferString&	operator=(double);

   BufferString&	operator+=(const char*);
   BufferString&	operator+=(int);
   BufferString&	operator+=(float);
   BufferString&	operator+=(double);

   unsigned int		size() const;
   char&		lastChar()		{ return buf_[size()-1]; }
   const char&		lastChar() const	{ return buf_[size()-1]; }
   inline unsigned int	bufSize() const		{ return len_; }
   void			setBufSize(unsigned int);
   inline unsigned int	minBufSize() const	{ return minlen_; }
   void			setMinBufSize(unsigned int);

   void			insertAt(int idx, const char*);
				//< If idx >= size(), pads spaces
   void			replaceAt(int idx, const char*,bool cutoff=true);
				//< If idx >= size(), pads spaces

   bool			operator >(const char*) const;
   bool			operator <(const char*) const;

   static const BufferString& empty();

protected:

    char*		buf_;
    unsigned int	len_;
    const unsigned int	minlen_;

private:

    void		init();
    inline void		destroy()	{ delete [] buf_; buf_ = 0; }

};

std::ostream& operator <<(std::ostream&,const BufferString&);
std::istream& operator >>(std::istream&,BufferString&);

inline bool BufferString::operator==( const BufferString& s ) const
{ return operator ==( s.buf_ ); }

inline bool BufferString::operator!=( const BufferString& s ) const
{ return operator !=( s.buf_ ); }

inline  bool BufferString::operator!=( const char* s ) const
{ return ! (*this == s); }




#endif

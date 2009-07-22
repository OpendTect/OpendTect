#ifndef rawarray_h
#define rawarray_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		1-9-95
 RCS:		$Id: rawarray.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

*/

#include <general.h>


/*!\brief Raw binary data access.  */

mClass RawDataArray
{
public:
			RawDataArray( int byts=4 )
			: nelem_(0), bytes_(byts), data_(0)
						{}

    inline int		size() const		{ return nelem_; }
    inline int		bytesPerSample() const	{ return bytes_; }
    bool		isZero() const;

    inline unsigned char*	data()		{ return data_; }
    inline const unsigned char* data() const	{ return data_; }

protected:

    int			nelem_;
    int			bytes_;
    unsigned char*	data_;

};


template <class T>
class DataArrayAccess : public RawDataArray
{
		DataArrayAccess( T* arr, int n, int s=1 )
		: RawDataArray(sizeof(T))
		{ nelem_ = n; data_ = (unsigned char*)arr; step = s; }

    T		operator[]( int idx ) const
		{ return *( ((T*)data_) + idx*step ); }
    T&		operator[]( int idx )
		{ return *( ((T*)data_) + idx*step ); }

protected:

    int		step;

};


#endif

#ifndef tracedata_h
#define tracedata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		10-5-1995
 RCS:		$Id: tracedata.h,v 1.10 2012-07-10 08:05:26 cvskris Exp $
________________________________________________________________________

-*/

#include "databuf.h"
#include "datainterp.h"

typedef DataInterpreter<float> TraceDataInterpreter;
class Scaler;


/*!\brief A set of data buffers and their interpreters.

A data buffer + interpreter is referred to as 'Component'. Note that this class
is not concernedabout what is contained in the buffers (descriptions,
constraints etc.).

*/


mClass TraceData
{
public:

			TraceData()
			: data_(0), interp_(0), nrcomp_(0)	{}
			TraceData( const TraceData& td )
			: data_(0), interp_(0), nrcomp_(0)	{ copyFrom(td);}
			~TraceData();
    bool		allOk() const;
    bool		isEmpty() const;

    inline TraceData&	operator=( const TraceData& td )
			{ copyFrom( td ); return *this; }
    void		copyFrom(const TraceData&);
			//!< copy all components, making an exact copy.
    void		copyFrom(const TraceData&,int comp_from,int comp_to);
			//!< copy comp_from of argument to my comp_to

    inline int		nrComponents() const
			{ return nrcomp_; }
    inline int		size( int icomp=0 ) const
			{ return icomp >= nrcomp_ ? 0 : data_[icomp]->size(); }
    inline int		bytesPerSample( int icomp=0 ) const
			{ return icomp >= nrcomp_ ? 1
			       : data_[icomp]->bytesPerSample(); }
    inline bool		isZero( int icomp=0 ) const
			{ return icomp >= nrcomp_ || data_[icomp]->isZero(); }

    inline float	getValue( int idx, int icomp=0 ) const
			{ return interp_[icomp]->get(data_[icomp]->data(),idx);}
    inline void	setValue( int idx, float v, int icomp=0 )
			{ interp_[icomp]->put( data_[icomp]->data(), idx, v ); }

    inline DataBuffer*			getComponent( int icomp=0)
					{ return data_[icomp]; }
    inline const DataBuffer*		getComponent( int icomp=0 ) const
					{ return data_[icomp]; }
    inline TraceDataInterpreter*	getInterpreter( int icomp=0 )
					{ return interp_[icomp]; }
    inline const TraceDataInterpreter*	getInterpreter( int icomp=0 ) const
					{ return interp_[icomp]; }

    void		addComponent(int ns,const DataCharacteristics&,
				     bool cleardata=false);
    void		delComponent(int);
    void		setComponent(const DataCharacteristics&,int icomp=0);

    void		reSize(int,int icomp=-1,bool copydata=false);
				//!< -1 = all data buffers
    void		scale(const Scaler&,int icomp=-1);
				//!< -1 = all data buffers
    void		zero(int icomp=-1);
				//!< -1 = all data buffers

    void		handleDataSwapping();
				//! pre-swaps all buffers that need it
	

protected:


    DataBuffer**	data_;
    TraceDataInterpreter** interp_;
    int			nrcomp_;

};


inline int clippedVal( float val, int lim )
{
    return val > (float)lim ? lim : (val < -((float)lim) ? -lim : mNINT32(val));
}


#endif

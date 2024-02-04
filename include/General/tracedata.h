#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "databuf.h"
#include "datainterp.h"

typedef DataInterpreter<float> TraceDataInterpreter;
class Scaler;


/*!\brief A set of data buffers and their interpreters.

A data buffer + interpreter is referred to as 'Component'. Note that this class
is not concernedabout what is contained in the buffers (descriptions,
constraints etc.).

*/


mExpClass(General) TraceData
{
public:

			TraceData();
			TraceData(const TraceData&);
    virtual		~TraceData();

    bool		allOk() const;
    bool		isEmpty() const;

    inline TraceData&	operator=( const TraceData& td )
			{ copyFrom( td ); return *this; }
    void		copyFrom(const TraceData&);
			//!< copy all components, making an exact copy.
    void		copyFrom(const TraceData&,int comp_from,int comp_to);
			//!< copy comp_from of argument to my comp_to
    void		convertTo(const DataCharacteristics&,
				  bool preserve_data=true);
    void		convertToFPs(bool preserve_data=true);

    inline int		nrComponents() const
			{ return nrcomp_; }
    inline int		size( int icomp=0 ) const
			{ return icomp >= nrcomp_ ? 0 : data_[icomp]->size(); }
    inline int		bytesPerSample( int icomp=0 ) const
			{ return icomp >= nrcomp_ ? 1
			       : data_[icomp]->bytesPerSample(); }
    inline bool		isZero( int icomp=0 ) const
			{ return icomp >= nrcomp_ || data_[icomp]->isZero(); }

    bool		isValidComp(int icomp=0) const;
    float		getValue(int isamp,int icomp=0) const;
    void		setValue(int isamp,float,int icomp=0);

    inline DataBuffer*	getComponent( int icomp=0 )
					{ return data_[icomp]; }
    inline const DataBuffer* getComponent( int icomp=0 ) const
					{ return data_[icomp]; }
    inline TraceDataInterpreter* getInterpreter( int icomp=0 )
					{ return interp_[icomp]; }
    inline const TraceDataInterpreter* getInterpreter( int icomp=0 ) const
					{ return interp_[icomp]; }

    void		addComponent(int ns,const DataCharacteristics&,
				     bool cleardata=false);
    void		delComponent(int);
    void		setComponent(const DataCharacteristics&,int icomp=0);
    void		setNrComponents(int nrcomp,
					DataCharacteristics::UserType);

    void		reSize(int,int icomp=-1,bool copydata=false);
				//!< -1 = all data buffers
    void		scale(const Scaler&,int icomp=-1);
				//!< -1 = all data buffers
    void		zero(int icomp=-1);
				//!< -1 = all data buffers

    void		handleDataSwapping();
				//! pre-swaps all buffers that need it


protected:


    DataBuffer**	data_					= nullptr;
    TraceDataInterpreter** interp_				= nullptr;
    int			nrcomp_					= 0;

};

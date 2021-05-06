#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Arnaud Huck
 Date:          Mar 2012
________________________________________________________________________

-*/

#include "generalmod.h"
#include "gendefs.h"

template <class T> class Array2D;
class BufferStringSet;
class DataPointSet;

mExpClass(General) HorVariogramComputer
{
public:

			HorVariogramComputer(DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    BufferString& errmsg,
					    bool msgiserror );
			~HorVariogramComputer();

    Array2D<float>*         getData() const;
    Array2D<float>*         getXaxes() const;
    BufferStringSet*        getLabels() const;

    bool                    isOK() const            { return dataisok_; }

protected:
    Array2D<float>*         variogramvals_;
    Array2D<float>*         axes_;
    BufferStringSet*        variogramnms_;

    bool                    dataisok_;

    bool                    compVarFromRange(DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    BufferString& errsmg,
					    bool msgiserror );
};


mExpClass(General) VertVariogramComputer
{
public:

			    VertVariogramComputer(DataPointSet& dpset,int,
						int step,int range,
						int fold, int nrgroups,
						BufferString& errmsg,
						bool msgiserror );
			    ~VertVariogramComputer();

    Array2D<float>*         getData() const;
    Array2D<float>*         getXaxes() const;
    Array2D<float>*         getStd() const;
    Array2D<od_int64>*      getFold() const;
    BufferStringSet*        getLabels() const;

    bool                    isOK() const            { return dataisok_; }

protected:
    Array2D<float>*         variogramvals_;
    Array2D<float>*         axes_;
    Array2D<float>*         variogramstds_;
    Array2D<od_int64>*      variogramfolds_;
    BufferStringSet*        variogramnms_;

    bool                    dataisok_;

    bool                    compVarFromRange(DataPointSet& dpset,int colid, 
					    int step,int range,int fold,
					    int nrgroups,
					    BufferString& errmsg,
					    bool msgiserror);

    struct MDandRowID
    {
	MDandRowID( double md=0, int rowid=0 )
	    : md_(md)
	    , rowid_(rowid)     {};

	double      md_;
	int         rowid_;

	bool        operator>( MDandRowID challenger ) const
		    { return md_ > challenger.md_; }
	bool        operator==( MDandRowID challenger ) const
		    { return md_ == challenger.md_; }
    };
};


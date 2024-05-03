#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "gendefs.h"
#include "uistring.h"

class BufferStringSet;
class DataPointSet;
namespace Stats { class RandGen; }
template <class T> class Array2D;

mExpClass(General) HorVariogramComputer
{ mODTextTranslationClass(HorVariogramComputer)
public:

			HorVariogramComputer(DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    uiString& errmsg,
					    bool msgiserror );
			~HorVariogramComputer();

    Array2D<float>*         getData() const;
    Array2D<float>*         getXaxes() const;
    BufferStringSet*        getLabels() const;

    bool                    isOK() const            { return dataisok_; }

private:
    Array2D<float>*         variogramvals_;
    Array2D<float>*         axes_;
    BufferStringSet*        variogramnms_;
    Stats::RandGen&	    gen_;

    bool                    dataisok_;

    bool                    compVarFromRange(DataPointSet& dpset, int size,
					    int cid, int range, int fold,
					    uiString& errsmg,
					    bool msgiserror );
};


mExpClass(General) VertVariogramComputer
{ mODTextTranslationClass(VertVariogramComputer)
public:

			    VertVariogramComputer(DataPointSet& dpset,int,
						int step,int range,
						int fold, int nrgroups,
						uiString& errmsg,
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

    bool		    compVarFromRange(DataPointSet& dpset,int colid,
					    int step,int range,int fold,
					    int nrgroups,
					    uiString& errmsg,
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

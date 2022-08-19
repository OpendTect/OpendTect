#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "paralleltask.h"

// MATLAB header files
#include <matrix.h>

template <class T> class ArrayND;


class ArrayNDCopier : public ParallelTask
{ mODTextTranslationClass(ArrayNDCopier)
public:
			ArrayNDCopier(const ArrayND<float>&);
			~ArrayNDCopier();
    bool		init(bool managemxarr);

    mxArray*		getMxArray()		{ return mxarr_; }

    uiString		uiNrDoneText() const	{ return tr("Values copied"); }

protected:

    od_int64		nrIterations() const	{ return totalnr_; }
    bool		doWork(od_int64 start,od_int64 stop,int thrid);

    mxArray*		mxarr_;
    const ArrayND<float>& arrnd_;
    od_int64		totalnr_;
    bool		managemxarr_;
};


class mxArrayCopier : public ParallelTask
{ mODTextTranslationClass(mxArrayCopier)
public:
			mxArrayCopier(const mxArray&,ArrayND<float>&);
			~mxArrayCopier();
    bool		init();
    uiString		uiNrDoneText() const	{ return tr("Values copied"); }

protected:

    od_int64		nrIterations() const	{ return totalnr_; }
    bool		doWork(od_int64 start,od_int64 stop,int thrid);

    const mxArray&	mxarr_;
    ArrayND<float>&	arrnd_;
    od_int64		totalnr_;
};

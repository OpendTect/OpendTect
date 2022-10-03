#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "namedobj.h"
#include "ptrman.h"
#include "uistring.h"

namespace Well
{
    class Data;
    class D2TModel;
}

namespace WellTie
{

mGlobal(WellAttrib) void calibrate(const Well::D2TModel&,Well::D2TModel&);

class DataWriter;
class Setup;

/*!
\brief Manages the D2T models used during TWTS.
*/

mExpClass(WellAttrib) D2TModelMgr
{ mODTextTranslationClass(D2TModelMgr);
public:
			D2TModelMgr(Well::Data&,DataWriter&,
				    const Setup&);
			~D2TModelMgr();

    bool		undo();
    bool		cancel();

    bool		updateFromWD();
    bool		commitToWD();

    void		setWD( Well::Data& wd ) { wd_ = &wd; }
    void		shiftModel(float);
    void		setAsCurrent(Well::D2TModel*);
    void		setFromData(float* dah,float* time,int sz);

    uiString		errMsg() const { return errmsg_; }

protected:

    RefMan<Well::Data>	wd_;

    Well::D2TModel*	d2T();
    Well::D2TModel*	prvd2t_ = nullptr;
    Well::D2TModel*	orgd2t_ = nullptr;

    uiString		errmsg_;

    DataWriter&		datawriter_;
    bool		emptyoninit_ = false;

};

} // namespace WellTie

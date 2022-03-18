#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltied2tmodelmanager.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "namedobj.h"
#include "uistring.h"
#include "ptrman.h"

namespace Well
{
    class Data;
    class D2TModel;
}

namespace WellTie
{

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
    Well::D2TModel*	prvd2t_;
    Well::D2TModel*	orgd2t_;

    uiString		errmsg_;

    DataWriter&		datawriter_;
    bool		emptyoninit_;

};

} // namespace WellTie


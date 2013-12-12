#ifndef fkfilterattrib_h
#define fkfilterattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2013
 RCS:           $Id$
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"

/*!\brief Applies FK filter */

namespace Attrib
{

mClass(ExpAttribs) FKFilter : public Provider
{
public:

    static void		initClass();
			FKFilter(Desc&);

    static const char*	attribName()	   { return "FKFilter"; }

protected:

    static Provider*	createInstance(Desc&);

    bool		getInputData(const BinID&,int);
    bool		computeData(const DataHolder&,const BinID&,
				    int,int,int) const;

    const DataHolder*	inpdata_;
};

} // namespace Attrib


#endif

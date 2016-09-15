#ifndef reliefattrib_h
#define reliefattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2016
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

namespace Attrib
{

/*!
\brief Pseudo %Relief Attribute
*/

mExpClass(Attributes) Relief: public Provider
{
public:
    static void		initClass();
			Relief(Desc&);

    static const char*	attribName()		{ return "Relief"; }

protected:
			~Relief() {}
    static Provider*	createInstance(Desc&);

    bool		allowParallelComputation() const;
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    int			dataidx_;
    const DataHolder*	inputdata_;
};

} // namespace Attrib

#endif

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "gendefs.h"

namespace Attrib
{

class DescSet;
class DescSetMan;
class DescSetsHolder;

mGlobal(AttributeEngine) const DescSetsHolder& DSHolder();
mGlobal(AttributeEngine) DescSetsHolder& eDSHolder();

/*!
\brief Pack to carry DescSet Managers for live attributes and DescSets for
stored data, both in 2D and 3D.
*/

mExpClass(AttributeEngine) DescSetsHolder
{
public:
				DescSetsHolder();
				~DescSetsHolder();

    const Attrib::DescSetMan*	getDescSetMan(bool is2d) const;
    Attrib::DescSetMan*		getDescSetMan(bool is2d);
    const Attrib::DescSet*	getDescSet(bool is2d,bool isstored) const;
    Attrib::DescSet*		getDescSet(bool is2d,bool isstored);
    void			replaceAttribSet(DescSet*);
				//this renews the manager

				//be sure you know what you're doing!
    void			replaceADSMan(DescSetMan*,bool dodelete=false);

protected:
    Attrib::DescSetMan*		adsman2d_;
    Attrib::DescSetMan*		adsman3d_;
    Attrib::DescSet*		adsstored2d_;
    Attrib::DescSet*		adsstored3d_;

    void			clearHolder();

    static DescSetsHolder*	dsholder_;

private:
    friend mExp(AttributeEngine)
	DescSetsHolder&		eDSHolder();
				//editable DescSetsHolder
    friend mExp(AttributeEngine)
	const DescSetsHolder&	DSHolder();

public:

    void			replaceStoredAttribSet(DescSet*);
				//needed for backward compatibility v < 4.1.1
};

} // namespace Attrib

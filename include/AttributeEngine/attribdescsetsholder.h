#ifndef attribdescsetsholder_h
#define attribdescsetsholder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene HUCK
 Date:          11-03-2010
 RCS:           $Id: attribdescsetsholder.h,v 1.3 2012-08-03 13:00:07 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "commondefs.h"

namespace Attrib 
{
class DescSet;
class DescSetMan;
class DescSetsHolder;

mGlobal(AttributeEngine) const DescSetsHolder& DSHolder();
mGlobal(AttributeEngine) DescSetsHolder& eDSHolder();

/*!\brief Pack to carry DescSet Managers for live attributes and DescSets
  for stored data, both in 2D and 3D*/

mClass(AttributeEngine) DescSetsHolder
{
public:
    				DescSetsHolder();
    				~DescSetsHolder();

    const Attrib::DescSetMan*	getDescSetMan(bool is2d) const;
    Attrib::DescSetMan*		getDescSetMan(bool is2d);
    const Attrib::DescSet*	getDescSet(bool is2d,bool isstored) const;
    Attrib::DescSet*		getDescSet(bool is2d,bool isstored);
    void                        replaceAttribSet(DescSet*);
    				//this renews the manager

    				//be sure you know what you're doing!
    void                        replaceADSMan(DescSetMan*,bool dodelete=false);

protected:
    Attrib::DescSetMan*		adsman2d_;
    Attrib::DescSetMan*		adsman3d_;
    Attrib::DescSet*		adsstored2d_;
    Attrib::DescSet*		adsstored3d_;

    void			clearHolder();

    static DescSetsHolder*	dsholder_;

private:
    friend DescSetsHolder&	eDSHolder();	//editable DescSetsHolder
    friend const DescSetsHolder&	DSHolder();

public:
    
    void                        replaceStoredAttribSet(DescSet*);
    				//needed for backward compatibility v < 4.1.1
};

} // namespace Attrib

#endif



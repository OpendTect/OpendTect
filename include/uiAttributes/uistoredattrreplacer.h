#ifndef uistoredattrreplacer_h
#define uistoredattrreplacer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		June 2008
 RCS:		$Id: uistoredattrreplacer.h,v 1.1 2008-06-18 08:19:58 cvssatyaki Exp $
________________________________________________________________________

-*/
#include "sets.h"

class uiParent;
class BufferStringSet;
namespace Attrib
{
    class Desc;
    class DescID;
    class DescSet;
};

class uiStoredAttribReplacer
{
public:
    				uiStoredAttribReplacer(uiParent*,
						       Attrib::DescSet&,bool);
protected:
	void 			replaceStoredAtrributes(bool);
	void			getUserRef(const Attrib::DescID&,
					   BufferStringSet&) const;
	void			getStoredIds();
	bool			hasInput(const Attrib::Desc&,
					 const Attrib::DescID&) const;
	Attrib::DescSet& 	attrset_;
	TypeSet<Attrib::DescID>	storedids_;
	bool		 	is2d_;
	uiParent*	 	parent_;
};

#endif


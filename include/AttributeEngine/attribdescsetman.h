#ifndef attribdescsetman_h
#define attribdescsetman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "multiid.h"

class IOPar;

namespace Attrib
{

class DescSet;

/*!
  \ingroup AttributeEngine
  \brief Manages attribute description set. 
*/

mExpClass(AttributeEngine) DescSetMan
{
public:

			DescSetMan(bool,DescSet* ads=0,bool destr_on_del=true );
			~DescSetMan();

    DescSet*		descSet()			{ return ads_; }
    const DescSet*	descSet() const			{ return ads_; }
    void		setDescSet(DescSet*);

    bool		is2D() const			{ return is2d_; }
    bool		isSaved() const          	{ return !unsaved_; }
    void		setSaved( bool yn=true ) const
                        { const_cast<DescSetMan*>(this)->unsaved_ = !yn; }
    bool&               unSaved()               	{ return unsaved_; }
                        //!< Added for convenience in UI building

    void		fillHist();
    IOPar&		inputHistory()			{ return inpselhist_; }

    MultiID		attrsetid_;

protected:

    DescSet*		ads_;
    bool		is2d_;
    bool		unsaved_;
    bool		destrondel_;

    IOPar&		inpselhist_;
    IOPar&		steerselhist_;

    void		cleanHist(IOPar&,const DescSet&);

};

}; // namespace Attrib

#endif


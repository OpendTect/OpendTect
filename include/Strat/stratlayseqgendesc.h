#ifndef stratlayseqgendesc_h
#define stratlayseqgendesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: stratlayseqgendesc.h,v 1.1 2010-10-13 11:27:57 cvsbert Exp $
________________________________________________________________________


-*/

#include "factory.h"
class IOPar;

namespace Strat
{
class LayerSequence;

/*!\brief Description that can generate layers to add to a sequence.

  genMaterial() needs a float, which must be in range [0,1]: the model position.
  When generating 11 sequences, these will be 0, 0.1, 0.2, ... 0.9, 1.

 */

mClass LayerGenDesc
{
public:	

    virtual bool	genMaterial(LayerSequence&,float modpos) const = 0;
    virtual const char*	errMsg() const				{ return 0; }
    virtual const char*	warnMsg() const				{ return 0; }

    virtual void	usePar(const IOPar&)			= 0;
    virtual void	fillPar(IOPar&) const			= 0;

    static LayerGenDesc* get(const IOPar&);
    mDefineFactoryInClass(LayerGenDesc,factory);

    virtual const char*	 userIdentification() const		= 0;
};


/*!\brief Collection of LayerGenDesc's that can form a full LayerSequence.  */

mClass LayerSequenceGenDesc : public ObjectSet<LayerGenDesc>
{
public:

			LayerSequenceGenDesc()		{}

    bool		getFrom(std::istream&);
    bool		putTo(std::ostream&) const;

    bool		generate(LayerSequence&,float modpos) const;

    const char*		errMsg() const			{ return errmsg_; }
    const BufferStringSet& warnMsgs() const		{ return warnmsgs_; }

protected:

    mutable BufferString	errmsg_;
    mutable BufferStringSet	warnmsgs_;

};


}; // namespace Strat

#endif

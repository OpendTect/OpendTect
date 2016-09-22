#ifndef attribpresentationinfo_h
#define attribpresentationinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "surveysectionprinfo.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "datapack.h"

namespace Attrib { class SelSpec; }

mExpClass(AttributeEngine) AttribPresentationInfo
				: public SectionLayerPresentationInfo
{
public:
				AttribPresentationInfo();
				~AttribPresentationInfo();
    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    void			setSelSpec(const Attrib::SelSpec&);
    const Attrib::SelSpec&	getSelSpec() const	{ return *attrspec_; }
    void			setAttribDataPack(const DataPack::ID& dpid)
				{ attribdpid_ = dpid; }
    const DataPack::ID&		getAttribDataPack() const { return attribdpid_;}
    void			setColTab(const ColTab::Sequence& ct)
				{ attrcoltab_ = ct; }
    const ColTab::Sequence&	getColTab() const	{ return attrcoltab_; }
    void			setColTabMapper(const ColTab::MapperSetup& mpsu)
				{ attrmapper_ = mpsu; }
    const ColTab::MapperSetup&	getColTabMapper() const
				{ return attrmapper_; }


    static SectionLayerPresentationInfo*	createFrom(const IOPar&);
    static void					initClass();
    static const char*				sFactoryKey();
    static const char*				sAttribDataPackID();
protected:
    Attrib::SelSpec*		attrspec_;
    DataPack::ID		attribdpid_;
    ColTab::Sequence		attrcoltab_;
    ColTab::MapperSetup		attrmapper_;

};

#endif

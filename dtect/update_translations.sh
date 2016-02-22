#!/bin/bash
#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id$
#
#_______________________________________________________________________________

# The purpose of this script is to make a .pro file to tell lupdate which
# files to scan and for which languages a ts file has to be created.
# It will:
# - make  a copy of an od directory
# - replace mExpClass and mClass by class
# - replace *_static_tr( "myfunc", ... ) with static_func_myfunc( ... )
# - create lupdate.pro
# - run lupdate for plural (*en-us.ts) and non-plural translations (all others).
# - copy the new translations into the work-directory.
# - remove the temporary copy

if [ $# -lt 5 ]; then
    echo "Usage : $0 <sourcedir> <tsbasedir> <binarydir> <application> <lupdate> [--quiet]"
    exit 1
fi

quiet=off

if [ $# -gt 5 ] && [ "$6" == "--quiet" ]; then
    quiet=on
fi


sourcedir=$1
tsbasedir=$2
binarydir=$3
application=$4
lupdate=$5
tmpoddir=/tmp/lupdate_tmp_$$
outputdir=${binarydir}/data/localizations/generated

scriptdir=`dirname $0`
scriptdir="${sourcedir}/${scriptdir}"
nrcpu=`${scriptdir}/GetNrProc`

removetmpoddirsed="s/\/tmp\/"
removetmpoddirsed+=`basename $tmpoddir`
removetmpoddirsed+="\///g"

kernel=`uname -a | awk '{print $1}'`
if [ "${kernel}" == "Darwin" ]; then
    lupdate_dir=`dirname ${lupdate}`
    export DYLD_LIBRARY_PATH=${lupdate_dir}/../lib
fi

if [ -e $tmpoddir ]; then
    \rm -rf $tmpoddir
fi

mkdir $tmpoddir

#Copy all src, include and pluginfiles (excluding CMake-stuff) to temp folder
olddir=`pwd`
cd ${sourcedir}
if [ -e ${sourcedir}/src ]; then
    find src -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/include ]; then
    find include -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/plugins ]; then
    find plugins -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi

projectdir="${tmpoddir}/data/localizations/source"
mkdir -p ${projectdir}

cd ${olddir}

shopt -s nullglob
#Copy existing ts-files ot project dir
cp -a ${tsbasedir}/data/localizations/source/${application}*.ts ${projectdir}
if [ -d ${tsbasedir}/data/localizations/inprogress ]; then
    cp -a ${tsbasedir}/data/localizations/inprogress/${application}*.ts ${projectdir}
fi

#Check if all projects exist in destination if so, check dates
missing=0
for fnm in ${projectdir}/${application}*.ts ; do
    basefnm=`basename ${fnm}`
    if [ ! -e "${outputdir}/${basefnm}" ]; then
	missing=1
    fi
done

if [ ${missing} -eq 0 ]; then

    #Get the newest file in the tree, and compare that to the oldest output
    oldest_output=`find ${outputdir} -name "${application}*.ts" -type f \
		    -printf '%T@ %p\n' \
		    | sort -rn \
		    | tail -1 \
		    | cut -f2- -d" "`

    newest_input=`find ${tmpoddir} \
		\( -path "*.h" -o -path "*.cc" -o -name "${application}*.ts" \) \
		-type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "`

    if [ -e "${newest_input}" ] && [ -e "${oldest_output}" ]; then
	if [ "${oldest_output}" -nt "${newest_input}" ]; then
	    if [ "${quiet}" == "off" ]; then
		echo "Nothing to do: Dependencies are older than target."
	    fi
	    #Remvoe temporary dir
	    rm -rf ${tmpoddir}
	    exit 0;
	fi
    fi
fi

profnm=${projectdir}/normaltrans.pro

cd ${projectdir}

headers=`find $tmpoddir -path "*.h"`
dirfile=${tmpoddir}/dirs
for header in ${headers}; do
    dirname ${header} >> ${dirfile}
done

dirs=`sort -u ${dirfile}`

#Create file with all files
filelist="filelist.txt"

echo "" >> ${filelist}

echo -n "HEADERS = " >> ${filelist}
for fnm in ${headers} ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

echo "" >> ${filelist}

sources=`find ${tmpoddir} -path "*.cc"`

echo -n "SOURCES = " >> ${filelist}
for fnm in $sources ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

# Create a list of target .ts files for normal ts files
echo -n "TRANSLATIONS = " > ${profnm}

echo " \\" >> ${profnm}
echo -n "    ${application}_template.ts" >> ${profnm}

nonomatch=1
for fnm in ${application}*.ts ; do
    #Don't run any plural here, as that is handled in plural project
    if [[ "${fnm}" =~ .*en-us.ts ]]; then
	continue
    fi

    #Is explicitly added above, so remove eventual entry from the list to avoid duplicates
    if [[ "${fnm}" =~ .*_template.ts ]]; then
	continue
    fi

    echo " \\" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
done

cat ${filelist} >> ${profnm}

echo "" >> ${profnm}
echo -n "INCLUDEPATH += " >> ${profnm}

for dir in ${dirs} ; do
    echo " \\" >> ${profnm}
    echo -n "	${dir}" >>${profnm}
done

#Create a list of .ts files for plural operations
pluralpro=$projectdir/plural.pro
pluralfile=${application}_en-us.ts
if [ -e ${pluralfile} ]; then
    echo -n "TRANSLATIONS = " > ${pluralpro}
    echo " \\" >> ${pluralpro}
    echo -n "    ${pluralfile}" >> ${pluralpro}
    cat ${filelist} >> ${pluralpro}

    echo "" >> ${pluralpro}
    echo -n "INCLUDEPATH += " >> ${pluralpro}

    for dir in ${dirs} ; do
	    echo " \\" >> ${pluralpro}
	    echo -n "	${dir}" >>${pluralpro}
    done
fi

#Remove the filelist
rm -rf ${filelist}

#Filter the sources for patterns
echo ${sources} | xargs -n 100 -P ${nrcpu} sed \
	-e 's/m.*TextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
	-e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
	-e 's/::tr(,/::tr(/g' \
	-e 's/__begquote"//g' \
	-e 's/"__endquote//g' -iTMP


#Filter away macros in sources
echo ${sources} | xargs -n 100 -P ${nrcpu} php ${scriptdir}/remove_macros.php


#Filter the headers for patterns
echo ${headers} | xargs -n 100 -P ${nrcpu} sed \
	-e 's/m.*TextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
        -e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
        -e 's/::tr(,/::tr(/g' \
        -e 's/__begquote"//g' \
        -e 's/"__endquote//g' -iTMP

#Filter away macros in headers
echo ${headers} | xargs -n 100 -P ${nrcpu} php ${scriptdir}/remove_macros.php

result=0
#Run lupdate in the background and save jobid
errfile=${tmpoddir}/stderr
${lupdate} -silent -locations relative ${profnm} 2> ${errfile} &
update_pid=$!

if [ -e ${pluralpro} ]; then
    #Run this in the forground, and the other job is going in the background
    plural_errfile=${tmpoddir}/stderr_plural
    ${lupdate} -silent -locations relative -pluralonly ${pluralpro} 2> ${plural_errfile}
    errors=`cat ${plural_errfile}`
    if [ "${errors}" != "" ]; then
	echo "Errors during plural processing:"
	cat ${plural_errfile} | sed -e $removetmpoddirsed -e 's/Q_OBJECT/*TextTranslationClass/g'
	result=1
    fi

    #Test if unfinished entries are found
    grep 'type="unfinished"' --before-context=1 ${application}_en-us.ts > /dev/null
    if [ $? -eq 0 ]; then
	echo "The following plural phrase(s) are not translated:"
	grep 'type="unfinished"' --before-context=1 ${application}_en-us.ts \
			| head -1 \
			| sed 's/<source>//g' \
			| sed 's/<\/source>//g' \
			| sed 's/ //g'
	echo
	echo "Please run:"
	echo
	echo  1. cp ${outputdir}/${application}_en-us.ts ${tsbasedir}/data/localizations/source/${application}_en-us.ts
	echo  2. ${scriptdir}/linguist.csh ${tsbasedir}/data/localizations/source/${application}_en-us.ts
	echo  3. Fix the problem
	echo  4. Commit
	echo
	result=1
    fi
fi

#Wait for background job to complete
wait ${update_pid}

errors=`cat ${errfile}`
if [ "${errors}" != "" ]; then
    echo "Errors during non-plural processing:"
    cat ${errfile} | sed -e $removetmpoddirsed -e 's/Q_OBJECT/*TextTranslationClass/g'
    result=1
fi

#Copy results back
rm -f ${outputdir}/${application}*.ts
rsync *.ts ${outputdir}/

#Remove temporary dir
rm -rf ${tmpoddir}

#Go back to starting dir
cd ${olddir}

exit ${result}

#!/bin/csh

set compdir=${1}

next:

shift

if ( $#argv<1 ) then
    goto the_end
endif

set file=${1}
gzip -c "${file}" > "${compdir}/${file}"

goto next

the_end:

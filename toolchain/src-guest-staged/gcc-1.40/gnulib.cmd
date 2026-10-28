del L_*.obj
del libgnu.lib
call bcc.cmd L_eprintf
call bcc.cmd L_umulsi3
call bcc.cmd L_mulsi3
call bcc.cmd L_udivsi3
call bcc.cmd L_divsi3
call bcc.cmd L_umodsi3
call bcc.cmd L_modsi3
call bcc.cmd L_lshrsi3
call bcc.cmd L_lshlsi3
call bcc.cmd L_ashrsi3
call bcc.cmd L_ashlsi3
call bcc.cmd L_divdf3
call bcc.cmd L_muldf3
call bcc.cmd L_negdf2
call bcc.cmd L_adddf3
call bcc.cmd L_subdf3
call bcc.cmd L_cmpdf2
call bcc.cmd L_fixunsdfsi
call bcc.cmd L_fixdfsi
call bcc.cmd L_floatsidf
call bcc.cmd L_addsf3
call bcc.cmd L_negsf2
call bcc.cmd L_subsf3
call bcc.cmd L_cmpsf2
call bcc.cmd L_mulsf3
call bcc.cmd L_divsf3
call bcc.cmd L_truncdfsf2
call bcc.cmd L_extendsfdf2
call bcc.cmd L_bb
call bcc.cmd L_builtin_new
call bcc.cmd L_builtin_New2
call bcc.cmd L_builtin_del
lib -out:bin\i386\gnulib.lib L_*.obj
del L_*.obj
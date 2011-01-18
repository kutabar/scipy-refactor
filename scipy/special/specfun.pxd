cimport numpy as np
from fwrap_ktp cimport *
cimport specfun_fc as fc

cpdef object lqmn(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t x, object qm=*, object qd=*)
cpdef object clpmn(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t x, fwr_dbl_t y, object cpm=*, object cpd=*)
cpdef object jdzo(fwi_integer_t nt, object n=*, object m=*, object pcode=*, object zo=*)
cpdef object bernob(fwi_integer_t n, object bn=*)
cpdef object bernoa(fwi_integer_t n, object bn=*)
cpdef object csphjy(fwi_integer_t n, fwc_complex_x16_t z, object csj=*, object cdj=*, object csy=*, object cdy=*)
cpdef object lpmns(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t x, object pm=*, object pd=*)
cpdef object eulera(fwi_integer_t n, object en=*)
cpdef object clqn(fwi_integer_t n, fwc_complex_x16_t z, object cqn=*, object cqd=*)
cpdef object airyzo(fwi_integer_t nt, fwi_integer_t kf=*, object xa=*, object xb=*, object xc=*, object xd=*)
cpdef object eulerb(fwi_integer_t n, object en=*)
cpdef object cva1(fwi_integer_t kd, fwi_integer_t m, fwr_dbl_t q, object cv=*)
cpdef object lqnb(fwi_integer_t n, fwr_dbl_t x, object qn=*, object qd=*)
cpdef object lamv(fwr_dbl_t v, fwr_dbl_t x, object vl=*, object dl=*)
cpdef object lagzo(fwi_integer_t n, object x=*, object w=*)
cpdef object legzo(fwi_integer_t n, object x=*, object w=*)
cpdef object pbdv(fwr_dbl_t v, fwr_dbl_t x, object dv=*, object dp=*)
cpdef object cerzo(fwi_integer_t nt, object zo=*)
cpdef object lamn(fwi_integer_t n, fwr_dbl_t x, object bl=*, object dl=*)
cpdef object clpn(fwi_integer_t n, fwc_complex_x16_t z, object cpn=*, object cpd=*)
cpdef object lqmns(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t x, object qm=*, object qd=*)
cpdef object chgm(fwr_dbl_t a, fwr_dbl_t b, fwr_dbl_t x)
cpdef object lpmn(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t x, object pm=*, object pd=*)
cpdef object fcszo(fwi_integer_t kf, fwi_integer_t nt, object zo=*)
cpdef object aswfb(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t c, fwr_dbl_t x, fwi_integer_t kd, fwr_dbl_t cv)
cpdef object lqna(fwi_integer_t n, fwr_dbl_t x, object qn=*, object qd=*)
cpdef object cpbdn(fwi_integer_t n, fwc_complex_x16_t z, object cpb=*, object cpd=*)
cpdef object lpn(fwi_integer_t n, fwr_dbl_t x, object pn=*, object pd=*)
cpdef object fcoef(fwi_integer_t kd, fwi_integer_t m, fwr_dbl_t q, fwr_dbl_t a, object fc__=*)
cpdef object sphi(fwi_integer_t n, fwr_dbl_t x, object si=*, object di=*)
cpdef object rcty(fwi_integer_t n, fwr_dbl_t x, object ry=*, object dy=*)
cpdef object lpni(fwi_integer_t n, fwr_dbl_t x, object pn=*, object pd=*, object pl=*)
cpdef object cyzo(fwi_integer_t nt, fwi_integer_t kf, fwi_integer_t kc, object zo=*, object zv=*)
cpdef object csphik(fwi_integer_t n, fwc_complex_x16_t z, object csi=*, object cdi=*, object csk=*, object cdk=*)
cpdef object sphj(fwi_integer_t n, fwr_dbl_t x, object sj=*, object dj=*)
cpdef object othpl(fwi_integer_t kf, fwi_integer_t n, fwr_dbl_t x, object pl=*, object dpl=*)
cpdef object klvnzo(fwi_integer_t nt, fwi_integer_t kd, object zo=*)
cpdef object jyzo(fwi_integer_t n, fwi_integer_t nt, object rj0=*, object rj1=*, object ry0=*, object ry1=*)
cpdef object rctj(fwi_integer_t n, fwr_dbl_t x, object rj=*, object dj=*)
cpdef object herzo(fwi_integer_t n, object x=*, object w=*)
cpdef object sphk(fwi_integer_t n, fwr_dbl_t x, object sk=*, object dk=*)
cpdef object pbvv(fwr_dbl_t v, fwr_dbl_t x, object vv=*, object vp=*)
cpdef object clqmn(fwi_integer_t m, fwi_integer_t n, fwc_complex_x16_t z, object cqm=*, object cqd=*)
cpdef object segv(fwi_integer_t m, fwi_integer_t n, fwr_dbl_t c, fwi_integer_t kd, object eg=*)
cpdef object sphy(fwi_integer_t n, fwr_dbl_t x, object sy=*, object dy=*)
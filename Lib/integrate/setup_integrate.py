#!/usr/bin/env python

import os
from glob import glob
import warnings

from scipy_distutils.core import Extension
from scipy_distutils.misc_util import get_path, default_config_dict, dot_join
from scipy_distutils.misc_util import fortran_library_item
from scipy_distutils.system_info import get_info,dict_append,\
     AtlasNotFoundError,BlasNotFoundError,BlasSrcNotFoundError

def configuration(parent_package='',parent_path=None):
    package = 'integrate'
    config = default_config_dict(package,parent_package)

    local_path = get_path(__name__,parent_path)

    atlas_info = get_info('atlas')

    f_libs = []
    if not atlas_info:
        warnings.warn(AtlasNotFoundError.__doc__)
        blas_info = get_info('blas')
        #blas_info = {} # test building BLAS from sources.
        if not blas_info:
            warnings.warn(BlasNotFoundError.__doc__)
            blas_src_info = get_info('blas_src')
            if not blas_src_info:
                raise BlasSrcNotFoundError,BlasSrcNotFoundError.__doc__
            dict_append(blas_info,libraries=['blas_src'])
        atlas_info = blas_info
        f_libs.append(fortran_library_item(\
                'blas_src',blas_src_info['sources']
                ))

    atlas_library_dirs = atlas_info.get('library_dirs',[])
    atlas_libraries = atlas_info.get('libraries',[])
    blas_libraries = atlas_libraries

    quadpack = glob(os.path.join(local_path,'quadpack','*.f'))
    f_libs.append(fortran_library_item(\
        'quadpack',quadpack,libraries = ['linpack_lite','mach']))

    odepack = glob(os.path.join(local_path,'odepack','*.f'))
    f_libs.append(fortran_library_item(\
        'odepack',odepack,
        libraries = ['linpack_lite']+blas_libraries,
        library_dirs = atlas_library_dirs))

    # should we try to weed through files and replace with calls to
    # LAPACK routines?
    # Yes, someday...
    linpack_lite = glob(os.path.join(local_path,'linpack_lite','*.f'))
    f_libs.append(fortran_library_item('linpack_lite',linpack_lite))

    mach = glob(os.path.join(local_path,'mach','*.f'))
    f_libs.append(fortran_library_item('mach',mach))

    # Extensions
    # quadpack
    sources = ['_quadpackmodule.c']
    sources = [os.path.join(local_path,x) for x in sources]
    ext = Extension(dot_join(parent_package,package,'_quadpack'),sources,
                    libraries=['quadpack'])
    config['ext_modules'].append(ext)

    # odepack
    sources = ['_odepackmodule.c']
    sources = [os.path.join(local_path,x) for x in sources]
    ext = Extension(dot_join(parent_package,package,'_odepack'),sources,
                    libraries=['odepack','mach'])
    config['ext_modules'].append(ext)

    # vode
    sources = [os.path.join(local_path,'vode.pyf')]
    ext = Extension(dot_join(parent_package,package,'vode'),
                    sources,
                    library_dirs=atlas_library_dirs,
                    libraries=['odepack'])
    config['ext_modules'].append(ext)

    config['fortran_libraries'].extend(f_libs)

    return config

if __name__ == '__main__':    
    from scipy_distutils.core import setup
    setup(**configuration())

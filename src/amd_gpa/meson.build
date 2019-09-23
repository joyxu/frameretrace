amdgpa_inc = include_directories('.')
amdgpa_lib = static_library('amdgpa', 'glframe_metrics_amd_gpa_stub.cpp',
                           include_directories : frameretrace_inc)

amdgpa_dep = declare_dependency(link_with : amdgpa_lib,
                                include_directories : amdgpa_inc)


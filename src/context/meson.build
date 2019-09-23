context_inc = include_directories('.')
context_lib = static_library('context', 'glframe_thread_context.cpp',
                             include_directories : [frameretrace_inc,
                                                    apitrace_inc])

context_dep = declare_dependency(link_with : context_lib,
                                 include_directories : context_inc)


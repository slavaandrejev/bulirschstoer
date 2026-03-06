bpc    = find_program('blueprint-compiler')
cppgir = find_program('cppgir')

fs    = import('fs')
gnome = import('gnome')

cppgir_dep    = dependency('cppgir')
gio_unix_dep  = dependency('gio-unix-2.0')
glbinding_dep = dependency('glbinding', method: 'cmake')
glm_dep       = dependency('glm', method: 'cmake')
gthread_dep   = dependency('gthread-2.0')
gtk_dep       = dependency('gtk4')
gtk_print_dep = dependency('gtk4-unix-print')
sigcpp_dep    = dependency('sigc++-3.0')

gtk_binding_names = [
    'cairo'
  , 'freetype2'
  , 'gdk'
  , 'gdkpixbuf'
  , 'gio'
  , 'glib'
  , 'gmodule'
  , 'gobject'
  , 'graphene'
  , 'gsk'
  , 'gtk'
  , 'harfbuzz'
  , 'pango'
  , 'pangocairo'
  ]

gen_bindings = custom_target(
    output: gtk_binding_names
  , command: [
        cppgir
      , '--output', '@OUTDIR@'
      , '--class'
      , 'Gtk-4.0'
      ]
  )
touch = find_program('touch')
gen_bindings_dummy_h = custom_target(
    output: 'gen_bindings_dummy.h'
  , command: [touch, '@OUTPUT@']
  , input: gen_bindings
  )

binding_src = []
binding_inc = []
foreach b : gtk_binding_names
    run_command('mkdir', meson.current_build_dir() / b, check: false)
    binding_src += custom_target(
        input: gen_bindings_dummy_h
      , output: f'@b@.cpp'
      , command: [
            'cp'
          , '@OUTDIR@' / b / f'@b@.cpp'
          , '@OUTDIR@' / f'@b@.cpp'
          ]
      )
    binding_inc += include_directories(b)
endforeach

gigtk_lib = static_library(
    'gigtk'
  , binding_src
  , gen_bindings_dummy_h
  , include_directories: [
        binding_inc
      ]
  , dependencies: [
        cppgir_dep
      , gio_unix_dep
      , gthread_dep
      , gtk_dep
      , gtk_print_dep
      ]
  )

gigtk_dep = declare_dependency(
    sources: gen_bindings_dummy_h
  , link_with: gigtk_lib
  , dependencies: [
        cppgir_dep
      , gio_unix_dep
      , gthread_dep
      , gtk_dep
      , gtk_print_dep
      ]
  )

blp_files = ['three-body-animation-win.blp']
ui_files = []
foreach f : blp_files
    ui_files += [fs.replace_suffix(f, '.ui')]
endforeach
blueprints = custom_target('blueprints',
    input:   blp_files
  , output:  ui_files
  , command: [bpc, 'batch-compile', '@OUTDIR@', '@CURRENT_SOURCE_DIR@', '@INPUT@']
  )

gtk_files = gnome.compile_resources(
    'three-body-animation-resources'
  , 'three-body-animation.gresource.xml'
  , c_name: 'three_body_animation'
  , dependencies: blueprints
  )

executable(
    'three-body-animation'
  , 'three-body-animation.cpp'
  , 'three-body-animation-win.cpp'
  , 'three-body-glrender.cpp'
  , gtk_files
  , include_directories: [project_inc]
  , dependencies : [
        eigen_dep
      , fmt_dep
      , glbinding_dep
      , glm_dep
      , gigtk_dep
      , sigcpp_dep
      ]
  )

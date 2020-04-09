# Several directory names used by libInstPatch to install files
# the variable names are similar to the KDE4 build system

include ( GNUInstallDirs )

# BUNDLE_INSTALL_DIR - Mac only: the directory for application bundles 
set (BUNDLE_INSTALL_DIR "/Applications" CACHE STRING 
     "The install dir for application bundles")
mark_as_advanced (BUNDLE_INSTALL_DIR)

# FRAMEWORK_INSTALL_DIR - Mac only: the directory for framework bundles
set (FRAMEWORK_INSTALL_DIR "/Library/Frameworks" CACHE STRING 
     "The install dir for framework bundles")
mark_as_advanced (FRAMEWORK_INSTALL_DIR)

# PLUGINS_DIR - the directory where application plugins will be installed
if ( UNIX )
  set (PLUGINS_DIR "${CMAKE_INSTALL_LIBDIR}/swami" CACHE STRING "The install dir for plugins")
  if ( NOT IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}" )
    set (PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR}")
  endif ()
else ( UNIX )
set (PLUGINS_DIR "plugins" CACHE STRING "The install dir for plugins")
endif ( UNIX )
mark_as_advanced (PLUGINS_DIR)

# IMAGES_DIR - the directory where images are installed
if ( UNIX )
  set (IMAGES_DIR "${CMAKE_INSTALL_DATADIR}/swami/images" CACHE STRING "The install dir for images")
  if ( NOT IS_ABSOLUTE "${CMAKE_INSTALL_DATADIR}" )
    set (IMAGES_DIR "${CMAKE_INSTALL_PREFIX}/${IMAGES_DIR}")
  endif ()
else ( UNIX )
set (IMAGES_DIR "images" CACHE STRING "The install dir for images")
endif ( UNIX )
mark_as_advanced (IMAGES_DIR)

# UIXML_DIR - the directory where UI XML is installed
if ( UNIX )
  set (UIXML_DIR "${CMAKE_INSTALL_DATADIR}/swami" CACHE STRING "The install dir for UI XML files")
  if ( NOT IS_ABSOLUTE "${CMAKE_INSTALL_DATADIR}" )
    set (UIXML_DIR "${CMAKE_INSTALL_PREFIX}/${UIXML_DIR}")
  endif ()
else ( UNIX )
set (UIXML_DIR "." CACHE STRING "The install dir for UI XML files")
endif ( UNIX )
mark_as_advanced (UIXML_DIR)

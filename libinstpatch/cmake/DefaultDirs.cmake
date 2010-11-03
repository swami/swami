# Several directory names used by libInstPatch to install files
# the variable names are similar to the KDE4 build system

# BUNDLE_INSTALL_DIR - Mac only: the directory for application bundles 
set (BUNDLE_INSTALL_DIR "/Applications" CACHE STRING 
     "The install dir for application bundles")
mark_as_advanced (BUNDLE_INSTALL_DIR)
     
# FRAMEWORK_INSTALL_DIR - Mac only: the directory for framework bundles
set (FRAMEWORK_INSTALL_DIR "/Library/Frameworks" CACHE STRING 
     "The install dir for framework bundles")
mark_as_advanced (FRAMEWORK_INSTALL_DIR) 

# BIN_INSTALL_DIR - the directory where executables will be installed
set (BIN_INSTALL_DIR "bin" CACHE STRING "The install dir for executables")
mark_as_advanced (BIN_INSTALL_DIR) 

# SBIN_INSTALL_DIR - the directory where system executables will be installed
set (SBIN_INSTALL_DIR "sbin" CACHE STRING 
     "The install dir for system executables")
mark_as_advanced (SBIN_INSTALL_DIR) 

# LIB_INSTALL_DIR - the directory where libraries will be installed
set (LIB_INSTALL_DIR "lib" CACHE STRING "The install dir for libraries")
mark_as_advanced (LIB_INSTALL_DIR) 

# INCLUDE_INSTALL_DIR - the install dir for header files
set (INCLUDE_INSTALL_DIR "include" CACHE STRING "The install dir for headers")
mark_as_advanced (INCLUDE_INSTALL_DIR) 

# DATA_INSTALL_DIR - the base install directory for data files
set (DATA_INSTALL_DIR "share" CACHE STRING 
     "The base install dir for data files")
mark_as_advanced (DATA_INSTALL_DIR) 

# DOC_INSTALL_DIR - the install dir for documentation
set (DOC_INSTALL_DIR "share/doc" CACHE STRING 
     "The install dir for documentation")
mark_as_advanced (DOC_INSTALL_DIR) 


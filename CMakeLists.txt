
########################################################
# Files

SET (keychainbridge_SRCS
     keychainbridge.cpp
     keychainbridgegui.cpp
)

SET (keychainbridge_UIS keychainbridgeguibase.ui)

SET (keychainbridge_MOC_HDRS
     keychainbridge.h
     keychainbridgegui.h
)

SET (keychainbridge_RCCS  keychainbridge.qrc)

########################################################
# Build

QT4_WRAP_UI (keychainbridge_UIS_H  ${keychainbridge_UIS})

QT4_WRAP_CPP (keychainbridge_MOC_SRCS  ${keychainbridge_MOC_HDRS})

QT4_ADD_RESOURCES(keychainbridge_RCC_SRCS ${keychainbridge_RCCS})

ADD_LIBRARY (keychainbridgeplugin MODULE ${keychainbridge_SRCS} ${keychainbridge_MOC_SRCS} ${keychainbridge_RCC_SRCS} ${keychainbridge_UIS_H})



set(QTKEYCHAIN_LIBRARY_NAMES qtkeychain)

find_library(QTKEYCHAIN_LIBRARY
    NAMES ${QTKEYCHAIN_LIBRARY_NAMES}
    PATHS
      ${LIB_DIR}
      $ENV{LIB}
      "$ENV{LIB_DIR}"
      /usr/local/lib
  )


find_path(QTKEYCHAIN_INCLUDE_DIR
    NAMES qtkeychain
    PATHS
      ${LIB_DIR}/include
      "$ENV{LIB_DIR}/include"
      $ENV{INCLUDE}
      /usr/local/include
      PATH_SUFFIXES qtkeychain/
  )


message(STATUS "Found QTKEYCHAIN_INCLUDE_DIR: " ${QTKEYCHAIN_INCLUDE_DIR})

message(STATUS "Found QTKEYCHAIN_LIBRARY: " ${QTKEYCHAIN_LIBRARY})
# /usr/local/lib/x86_64-linux-gnu/libqtkeychain.a


# include ui built headers
INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/src/ui
     ${QTKEYCHAIN_INCLUDE_DIR}
     ../../core ../../core/geometry ../../core/raster
     ../../gui
     ..
     ${QCA_INCLUDE_DIR}
)


TARGET_LINK_LIBRARIES(keychainbridgeplugin
  qgis_core
  qgis_gui
  ${QTKEYCHAIN_LIBRARY}
)


########################################################
# Install

INSTALL(TARGETS keychainbridgeplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})


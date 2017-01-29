########################################################
# Packages

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

FIND_PACKAGE(QtKeychain REQUIRED)

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

# for unit testing
IF(ENABLE_TESTS)
  ADD_LIBRARY (keychainbridgeplugin_static STATIC ${keychainbridge_SRCS} ${keychainbridge_MOC_SRCS} ${keychainbridge_RCC_SRCS} ${keychainbridge_UIS_H})
ENDIF(ENABLE_TESTS)

# shared includes
INCLUDE_DIRECTORIES (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
INCLUDE_DIRECTORIES (SYSTEM
  ${QCA_INCLUDE_DIR}
  ${QTKEYCHAIN_INCLUDE_DIR}
)

IF(WITH_DESKTOP)
  # when in QGIS src tree
  # include ui built headers
  INCLUDE_DIRECTORIES(
     ${CMAKE_BINARY_DIR}/src/ui
     ${CMAKE_SOURCE_DIR}/src/core
     ${CMAKE_SOURCE_DIR}/src/core/auth
     ${CMAKE_SOURCE_DIR}/src/core/geometry
     ${CMAKE_SOURCE_DIR}/src/core/raster
     ${CMAKE_SOURCE_DIR}/src/gui
     ${CMAKE_SOURCE_DIR}/src/plugins
     ${CMAKE_SOURCE_DIR}/src
  )

  SET(PLUGIN_TARGET_LIBS
    qgis_core
    qgis_gui
    ${QTKEYCHAIN_LIBRARY}
  )
ELSE(WITH_DESKTOP)
  # when standalone (building against QGIS build/install)
  INCLUDE_DIRECTORIES (SYSTEM
    ${QT_INCLUDE_DIR}
    ${QGIS_INCLUDE_DIR}
  )

  SET(PLUGIN_TARGET_LIBS
    ${QGIS_CORE_LIBRARY}
    ${QGIS_GUI_LIBRARY}
    ${QT_QTMAIN_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTSVG_LIBRARY}
    ${QTKEYCHAIN_LIBRARY}
  )
ENDIF(WITH_DESKTOP)


TARGET_LINK_LIBRARIES(keychainbridgeplugin
  ${PLUGIN_TARGET_LIBS}
)

IF(ENABLE_TESTS)
  TARGET_LINK_LIBRARIES(keychainbridgeplugin_static
    ${PLUGIN_TARGET_LIBS}
  )
ENDIF(ENABLE_TESTS)

########################################################
# Install

INSTALL(TARGETS keychainbridgeplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})


PROJECT(imageRegTool)


IF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )

  FIND_PACKAGE(ITK)
  IF(ITK_FOUND)
    INCLUDE(${ITK_USE_FILE})
  ELSE(ITK_FOUND)
    MESSAGE(FATAL_ERROR
            "Cannot build ITKApps without ITK.  Please set ITK_DIR.")
  ENDIF(ITK_FOUND)

  FIND_PACKAGE(FLTK)
  IF(FLTK_FOUND)
    INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})
  ENDIF(FLTK_FOUND)

ENDIF( BUILD_OUTSIDE_INSIGHT_APPLICATIONS )


IF(UNIX)
  INCLUDE( ${CMAKE_ROOT}/Modules/FindX11.cmake )
ENDIF(UNIX)
CONFIGURE_FILE(${ITKApps_SOURCE_DIR}/LandmarkInitializedMutualInformationRegistration/Help/Index.html
               ${ITKApps_BINARY_DIR}/LandmarkInitializedMutualInformationRegistration/Help/Index.html COPYONLY)

INCLUDE_DIRECTORIES(
  ${ITKApps_SOURCE_DIR}/Auxiliary/FltkImageViewer
  ${ITKApps_BINARY_DIR}/Auxiliary/FltkImageViewer
  ${ITKApps_SOURCE_DIR}/Auxiliary/ITKFlFileWriter
  ${ITKApps_SOURCE_DIR}/LandmarkInitializedMutualInformationRegistration
  ${ITKApps_BINARY_DIR}/LandmarkInitializedMutualInformationRegistration
)

FLTK_WRAP_UI(imageRegTool guiMain.fl)
ITK_DISABLE_FLTK_GENERATED_WARNINGS(guiMain.fl)

ADD_EXECUTABLE(imageRegTool
    guiMainImplementation.cxx
    LandmarkRegistrator.cxx
    main.cxx
    ${imageRegTool_FLTK_UI_SRCS}
    )
INSTALL_TARGETS(/bin imageRegTool)

TARGET_LINK_LIBRARIES (imageRegTool
  ${ITK_LIBRARIES}
  ITKFlFileWriter
  ITKFltkImageViewer
)


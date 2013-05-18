#-----------------------------------------------------------------------------
# Get and build VTK
#
#-----------------------------------------------------------------------------

set( VTK_TAG "v5.10.1" )

ExternalProject_Add(VTK
  GIT_REPOSITORY "${git_protocol}://vtk.org/VTK.git"
  GIT_TAG "${VTK_TAG}"
  SOURCE_DIR VTK
  BINARY_DIR VTK-build
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    ${ep_common_args}
    -DBUILD_SHARED_LIBS:BOOL=TRUE
    -DVTK_DEBUG_LEAKS:BOOL=TRUE
    -DVTK_WRAP_TCL:BOOL=${ENABLE_TCL_WRAPPED_APPS}
    -DBUILD_EXAMPLES:BOOL=OFF
    -DBUILD_TESTING:BOOL=OFF
  INSTALL_COMMAND ""
)

set(VTK_DIR ${CMAKE_BINARY_DIR}/VTK-build)
set(VTK_EXTERNAL_PROJECT "TRUE")

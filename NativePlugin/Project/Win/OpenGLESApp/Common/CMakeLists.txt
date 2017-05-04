
set ( common_src Source/esShader.c 
                 Source/esShapes.c
                 Source/esTransform.c
                 Source/esUtil.c )

SET(
	ADDIT_INCLUDE_DIR
	"Include"
	"../Include"
	)

include_directories(${ADDIT_INCLUDE_DIR})

# Win32 Platform files
if(WIN32)
    set( common_platform_src Source/Win32/esUtil_win32.c )
    add_library( Common STATIC ${common_src} ${common_platform_src} )
    target_link_libraries( Common ${CMAKE_CURRENT_SOURCE_DIR}/../libs/libEGL.lib ${CMAKE_CURRENT_SOURCE_DIR}/../libs/libGLESv2.lib )
else()
    find_package(X11)
    find_library(M_LIB m)
    set( common_platform_src Source/LinuxX11/esUtil_X11.c )
    add_library( Common STATIC ${common_src} ${common_platform_src} )
    target_link_libraries( Common ${OPENGLES3_LIBRARY} ${EGL_LIBRARY} ${X11_LIBRARIES} ${M_LIB} )
endif()

             



cmake_minimum_required(VERSION 3.8)

SET(
	ADDIT_INCLUDE_DIR
	"Common/Include"
	"Include"
	)

include_directories(${ADDIT_INCLUDE_DIR})

add_executable( OpenGLESApp MainApp.cpp )
if(MSVC)
	set_target_properties(OpenGLESApp PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
	set_target_properties(OpenGLESApp PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
ENDIF(MSVC)
target_link_libraries( OpenGLESApp NativeParticleSystem Common)
cmake_minimum_required(VERSION 3.8)

SET(
	ADDIT_INCLUDE_DIR
	"Common/Include"
	"Include"
	)

include_directories(${ADDIT_INCLUDE_DIR})

add_executable( OpenGLESApp MainApp.cpp )
target_link_libraries( OpenGLESApp NativeParticleSystem Common)

IF(MSVC)
	add_custom_command(TARGET OpenGLESApp POST_BUILD
	COMMAND Set COPYCMD=/Y
    COMMAND xcopy ..\\..\\..\\..\\..\\Project\\Win\\OpenGLESApp\\libs ..\\..\\..\\Build\\Debug /s /e
    ) 
ENDIF(MSVC)
cmake_minimum_required(VERSION 3.8)
#MESSAGE(STATUS "This is TestHttpClient_SOURCE_DIR="${NativeParticleSystem_SOURCE_DIR})  
MESSAGE(STATUS "This is CMAKE_SOURCE_DIR=" ${CMAKE_SOURCE_DIR})  
MESSAGE(STATUS "This is CMAKE_CURRENT_SOURCE_DIR=" ${CMAKE_CURRENT_SOURCE_DIR})  
PROJECT (NativeParticleSystem)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_VERBOSE_MAKEFILE ON)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Build)

SET(
	ADDIT_INCLUDE_DIR
	"Src"
	"Include"
	"Include/Precomplie"
	"Src/Runtime"
	"./"
	)

#SET(GFX_SUPPORTS_OPENGL_UNIFIED ON CACHE BOOL "Support Opengles")
SET(ENABLE_MULTITHREADED_PARTICLES ON CACHE BOOL "Support multi-thread particles")

IF(GFX_SUPPORTS_OPENGL_UNIFIED)
	SET(
		OPENGLES_SRC
		"Src/Runtime/GfxDevice/opengles/GfxDeviceGLES.h"
		"Src/Runtime/GfxDevice/opengles/GfxDeviceGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiEnumGLES.h"
		"Src/Runtime/GfxDevice/opengles/ApiEnumGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiFuncGLES.h"
		"Src/Runtime/GfxDevice/opengles/ApiFuncGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiGLES.h"
		"Src/Runtime/GfxDevice/opengles/ApiGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiTranslateGLES.h"
		"Src/Runtime/GfxDevice/opengles/ApiTranslateGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiTypeGLES.h"
		"Src/Runtime/GfxDevice/opengles/BuffersGLES.h"
		"Src/Runtime/GfxDevice/opengles/AssertGLES.h"
		"Src/Runtime/GfxDevice/opengles/AssertGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/DeviceStateGLES.h"
		"Src/Runtime/GfxDevice/opengles/DeviceStateGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/ApiConstantsGLES.h"
		"Src/Runtime/GfxDevice/opengles/GraphicsCapsGLES.h"
		"Src/Runtime/GfxDevice/opengles/GraphicsCapsGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/GfxContextGLES.h"
		"Src/Runtime/GfxDevice/opengles/GfxContextGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/GfxGetProcAddressGLES.h"
		"Src/Runtime/GfxDevice/opengles/GfxGetProcAddressGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/HandleGLES.h"
		"Src/Runtime/GfxDevice/opengles/VertexDeclarationGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/VertexDeclarationGLES.h"
		"Src/Runtime/GfxDevice/opengles/DataBuffersGLES.h"
		"Src/Runtime/GfxDevice/opengles/DataBuffersGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/FrameBufferGLES.h"
		"Src/Runtime/GfxDevice/opengles/FrameBufferGLES.cpp"
		"Src/Runtime/GfxDevice/opengles/GfxDeviceResourcesGLES.h"

		"Src/Runtime/GfxDevice/GfxDeviceConfigure.h"
		"Src/Runtime/Utilities/fixed_array.h"
		)	
ELSE()
	SET(OPENGLES_SRC "")
ENDIF(GFX_SUPPORTS_OPENGL_UNIFIED)

IF(WIN32)
	SUBDIRS(Project/Win/D3DApp)
	SUBDIRS(Project/Win/OpenGLESApp
		Project/Win/OpenGLESApp/Common)

	SET(GFX_SUPPORTS_D3D9 ON CACHE BOOL "Support D3D9")
	CONFIGURE_FILE(config.h.in config.h)
ENDIF(WIN32)

IF(WIN32)
	SET(
		PLATFORM_INCLUDE
		"Include/Precomplie/VisualStudioPrefix.h"
		)
	
	SET(
		PLATFORM_SRC
		"Src/Runtime/GfxDevice/d3d/D3D9Context.h"
		"Src/Runtime/GfxDevice/d3d/D3D9Context.cpp"
		"Src/Runtime/GfxDevice/d3d/D3D9Enumeration.h"
		"Src/Runtime/GfxDevice/d3d/D3D9Includes.h"
		"Src/Runtime/GfxDevice/d3d/D3D9Utils.h"
		"Src/Runtime/GfxDevice/d3d/D3D9Utils.cpp"
		"Src/Runtime/GfxDevice/d3d/GfxDeviceD3D9.h"
		"Src/Runtime/GfxDevice/d3d/GfxDeviceD3D9.cpp"
		"Src/Runtime/GfxDevice/d3d/GraphicsCapsD3D9.cpp"
		"Src/Runtime/GfxDevice/d3d/IndexBufferD3D9.h"
		"Src/Runtime/GfxDevice/d3d/IndexBufferD3D9.cpp"
		"Src/Runtime/GfxDevice/d3d/ShaderConstantCacheD3D9.h"
		"Src/Runtime/GfxDevice/d3d/VertexBufferD3D9.h"
		"Src/Runtime/GfxDevice/d3d/VertexBufferD3D9.cpp"
		"Src/Runtime/GfxDevice/d3d/VertexDeclarationD3D9.h"
		"Src/Runtime/GfxDevice/d3d/VertexDeclarationD3D9.cpp"

		"Src/Runtime/Threads/Winapi/PlatformEvent.h"
		"Src/Runtime/Threads/Winapi/PlatformMutex.h"
		"Src/Runtime/Threads/Winapi/PlatformMutex.cpp"
		"Src/Runtime/Threads/Winapi/PlatformSemaphore.h"
		"Src/Runtime/Threads/Winapi/PlatformThread.h"
		"Src/Runtime/Threads/Winapi/PlatformThread.cpp"
		"Src/Runtime/Threads/Winapi/PlatformThreadSpecificValue.h"

		"Src/PlatformDependent/Win/WinSystemInfo.cpp"
		)
ELSE()
	SET(
		PLATFORM_SRC
		""
		)

	SET(
		PLATFORM_INCLUDE
		""
		)
ENDIF()

set(
	LIB_INCLUDE
	"Include/Configuration/UnityConfigure.h"
	"Include/Precomplie/PluginPrefix.h"
	"Include/Precomplie/PrefixConfigure.h"
	"Include/UnityPluginInterface.h"
	)

set(
	LIB_SRC 
	"Src/Mono/NativeUtil.h"
	"Src/Mono/NativeUtil.cpp"
	"Src/Mono/ScriptingAPI.h"
	"Src/Mono/ScriptingAPI.cpp"
	"Src/Mono/ScriptingAPI_Mono.cpp"
	"Src/Mono/ScriptingTypes.h"
	"Src/RenderingPlugin.cpp"

	"Src/Runtime/Allocator/AllocationHeader.h"
	"Src/Runtime/Allocator/BaseAllocator.h"
	"Src/Runtime/Allocator/BaseAllocator.cpp"
	"Src/Runtime/Allocator/DynamicHeapAllocator.cpp"
	"Src/Runtime/Allocator/DynamicHeapAllocator.h"
	"Src/Runtime/Allocator/LinearAllocator.h"
	"Src/Runtime/Allocator/MemoryMacros.h"
	"Src/Runtime/Allocator/MemoryManager.cpp"
	"Src/Runtime/Allocator/MemoryManager.h"
	"Src/Runtime/Allocator/StackAllocator.h"
	"Src/Runtime/Allocator/STLAllocator.h"

	"Src/Runtime/GfxDevice/BuiltinShaderParams.h"
	"Src/Runtime/GfxDevice/BuiltinShaderParams.cpp"
	"Src/Runtime/GfxDevice/BuiltinShaderParamsNames.cpp"
	"Src/Runtime/GfxDevice/BuiltinShaderParamsNames.h"
	"Src/Runtime/GfxDevice/ChannelAssigns.h"
	"Src/Runtime/GfxDevice/ChannelAssigns.cpp"
	"Src/Runtime/GfxDevice/GeometryJob.h"
	"Src/Runtime/GfxDevice/GeometryJob.cpp"
	"Src/Runtime/GfxDevice/GfxBuffer.h"
	"Src/Runtime/GfxDevice/GfxDevice.h"
	"Src/Runtime/GfxDevice/GfxDevice.cpp"
	"Src/Runtime/GfxDevice/GfxDeviceResources.h"
	"Src/Runtime/GfxDevice/GfxDeviceTypes.h"
	"Src/Runtime/GfxDevice/TransformState.h"
	"Src/Runtime/GfxDevice/VertexDeclaration.h"
	"Src/Runtime/GfxDevice/VertexDeclaration.cpp"
	"Src/Runtime/GfxDevice/threaded/GfxCommands.h"
	"Src/Runtime/GfxDevice/threaded/GfxDeviceClient.h"
	"Src/Runtime/GfxDevice/threaded/GfxDeviceThreadedClientManager.h"
	"Src/Runtime/GfxDevice/threaded/GfxDeviceWorker.h"
	"Src/Runtime/GfxDevice/threaded/ThreadedBuffer.h"
	"Src/Runtime/GfxDevice/threaded/ThreadedDeviceStates.h"
	"Src/Runtime/GfxDevice/threaded/ThreadedDynamicVBO.h"
	"Src/Runtime/GfxDevice/threaded/ThreadedVertexDeclaration.h"

	"Src/Runtime/Graphics/Renderer.h"
	"Src/Runtime/Graphics/Renderer.cpp"
	"Src/Runtime/Graphics/Transform.h"
	"Src/Runtime/Graphics/Mesh/DynamicVBO.h"
	"Src/Runtime/Graphics/Mesh/DynamicVBO.cpp"
	"Src/Runtime/Graphics/Mesh/GenericDynamicVBO.h"
	"Src/Runtime/Graphics/Mesh/GenericDynamicVBO.cpp"
	"Src/Runtime/Graphics/Mesh/Mesh.h"
	"Src/Runtime/Graphics/Mesh/MeshVertexFormat.h"
	"Src/Runtime/Graphics/Mesh/MeshVertexFormat.cpp"
	"Src/Runtime/Graphics/Mesh/ShaderChannels.h"
	"Src/Runtime/Graphics/Mesh/ShaderChannels.cpp"
	"Src/Runtime/Graphics/Mesh/SharedMeshData.h"
	"Src/Runtime/Graphics/Mesh/VertexData.h"
	"Src/Runtime/Graphics/Mesh/VertexData.cpp"

	"Src/Runtime/Input/TimeManager.h"
	"Src/Runtime/Input/TimeManager.cpp"

	"Src/Runtime/Jobs/BatchedForEach.h"
	"Src/Runtime/Jobs/BlockRangeJob.h"
	"Src/Runtime/Jobs/BlockRangeJob.cpp"
	"Src/Runtime/Jobs/JobBatchDispatcher.h"
	"Src/Runtime/Jobs/JobBatchDispatcher.cpp"
	"Src/Runtime/Jobs/Jobs.h"
	"Src/Runtime/Jobs/Jobs.cpp"
	"Src/Runtime/Jobs/JobSet.h"
	"Src/Runtime/Jobs/JobSet.cpp"
	"Src/Runtime/Jobs/JobSystem.h"
	"Src/Runtime/Jobs/JobSystem.cpp"
	"Src/Runtime/Jobs/jobtypes.h"
	"Src/Runtime/Jobs/Internal/JobGroupID.h"
	"Src/Runtime/Jobs/Internal/JobQueue.h"
	"Src/Runtime/Jobs/Internal/JobQueue.cpp"
	"Src/Runtime/Jobs/Internal/SingleThreadedJobQueue.h"
	"Src/Runtime/Jobs/Internal/SingleThreadedJobQueue.cpp"

	"Src/Runtime/Log/Log.h"
	"Src/Runtime/Log/Log.cpp"

	"Src/Runtime/Math/AnimationCurve.h"
	"Src/Runtime/Math/AnimationCurve.cpp"
	"Src/Runtime/Math/Color.h"
	"Src/Runtime/Math/FloatConversion.h"
	"Src/Runtime/Math/Gradient.h"
	"Src/Runtime/Math/Gradient.cpp"
	"Src/Runtime/Math/Matrix3x3.h"
	"Src/Runtime/Math/Matrix3x3.cpp"
	"Src/Runtime/Math/Matrix4x4.h"
	"Src/Runtime/Math/Matrix4x4.cpp"
	"Src/Runtime/Math/Matrix4x4_REF.cpp"
	"Src/Runtime/Math/Polynomials.h"
	"Src/Runtime/Math/Quaternion.h"
	"Src/Runtime/Math/Quaternion.cpp"
	"Src/Runtime/Math/Vector2.h"
	"Src/Runtime/Math/Vector2.cpp"
	"Src/Runtime/Math/Vector3.h"
	"Src/Runtime/Math/Vector3.cpp"
	"Src/Runtime/Math/Vector4.h"
	"Src/Runtime/Math/Random/rand.h"
	"Src/Runtime/Math/Random/rand.cpp"
	"Src/Runtime/Math/Random/Random.h"
	"Src/Runtime/Math/Simd/RotationOrder.h"
	"Src/Runtime/Math/Rect.h"

	"Src/Runtime/Misc/AllocatorLabelNames.h"
	"Src/Runtime/Misc/AllocatorLabels.h"
	"Src/Runtime/Misc/AllocatorLabels.cpp"
	"Src/Runtime/Misc/SystemInfo.h"
	"Src/Runtime/Misc/SystemInfo.cpp"

	"Src/Runtime/ParticleSystem/ColorModule.h"
	"Src/Runtime/ParticleSystem/ColorModule.cpp"
	"Src/Runtime/ParticleSystem/EmissionModule.h"
	"Src/Runtime/ParticleSystem/EmissionModule.cpp"
	"Src/Runtime/ParticleSystem/InitialModule.h"
	"Src/Runtime/ParticleSystem/InitialModule.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystem.h"
	"Src/Runtime/ParticleSystem/ParticleSystem.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemCommon.h"
	"Src/Runtime/ParticleSystem/ParticleSystemCurves.h"
	"Src/Runtime/ParticleSystem/ParticleSystemCurves.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemGradients.h"
	"Src/Runtime/ParticleSystem/ParticleSystemGradients.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemModule.h"
	"Src/Runtime/ParticleSystem/ParticleSystemModule.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemParticles.h"
	"Src/Runtime/ParticleSystem/ParticleSystemParticles.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemRenderer.h"
	"Src/Runtime/ParticleSystem/ParticleSystemRenderer.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemUtils.h"
	"Src/Runtime/ParticleSystem/ParticleSystemUtils.cpp"
	"Src/Runtime/ParticleSystem/PolynomialCurve.h"
	"Src/Runtime/ParticleSystem/PolynomialCurve.cpp"
	"Src/Runtime/ParticleSystem/RotationModule.h"
	"Src/Runtime/ParticleSystem/RotationModule.cpp"
	"Src/Runtime/ParticleSystem/RotationModule.h"
	"Src/Runtime/ParticleSystem/ShapeModule.h"
	"Src/Runtime/ParticleSystem/ShapeModule.cpp"
	"Src/Runtime/ParticleSystem/SizeModule.h"
	"Src/Runtime/ParticleSystem/SizeModule.cpp"
	"Src/Runtime/ParticleSystem/UVModule.h"
	"Src/Runtime/ParticleSystem/UVModule.cpp"
	"Src/Runtime/ParticleSystem/ParticleSystemGeometryJob.h"
	"Src/Runtime/ParticleSystem/ParticleSystemGeometryJob.cpp"

	"Src/Runtime/Shaders/GraphicsCaps.h"
	"Src/Runtime/Shaders/GraphicsCaps.cpp"

	"Src/Runtime/Threads/AtomicNode.h"
	"Src/Runtime/Threads/AtomicOps.h"
	"Src/Runtime/Threads/AtomicQueue.h"
	"Src/Runtime/Threads/AtomicQueue.cpp"
	"Src/Runtime/Threads/AtomicRefCounter.h"
	"Src/Runtime/Threads/CappedSemaphore.h"
	"Src/Runtime/Threads/ConcurrentContainers.h"
	"Src/Runtime/Threads/Event.h"
	"Src/Runtime/Threads/ExtendedAtomicOps.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-arm.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-arm64.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-ppc.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-ppc64.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-x86.h"
	"Src/Runtime/Threads/ExtendedAtomicOps-x86-64.h"
	"Src/Runtime/Threads/ExtendedAtomicTypes.h"
	"Src/Runtime/Threads/MultiWriterSingleReaderAtomicCircularBuffer.h"
	"Src/Runtime/Threads/MultiWriterSingleReaderAtomicCircularBuffer.cpp"
	"Src/Runtime/Threads/Mutex.h"
	"Src/Runtime/Threads/Mutex.cpp"
	"Src/Runtime/Threads/MutexLockedQueue.h"
	"Src/Runtime/Threads/MutexLockedQueue.cpp"
	"Src/Runtime/Threads/MutexLockedStack.h"
	"Src/Runtime/Threads/MutexLockedStack.cpp"
	"Src/Runtime/Threads/ProfilerMutex.h"
	"Src/Runtime/Threads/ReadWriteLock.h"
	"Src/Runtime/Threads/ReadWriteSpinLock.h"
	"Src/Runtime/Threads/Semaphore.h"
	"Src/Runtime/Threads/SimpleLock.h"
	"Src/Runtime/Threads/TaskChainProcessor.h"
	"Src/Runtime/Threads/TaskQueue.h"
	"Src/Runtime/Threads/Thread.h"
	"Src/Runtime/Threads/Thread.cpp"
	"Src/Runtime/Threads/ThreadedStreamBuffer.h"
	"Src/Runtime/Threads/ThreadedStreamBuffer.cpp"
	"Src/Runtime/Threads/ThreadHelper.h"
	"Src/Runtime/Threads/ThreadHelper.cpp"
	"Src/Runtime/Threads/ThreadSharedObject.h"
	"Src/Runtime/Threads/ThreadSpecificValue.h"
	"Src/Runtime/Threads/ThreadSpecificValue.cpp"
	"Src/Runtime/Threads/ThreadToCoreMappings.h"
	"Src/Runtime/Threads/ThreadUtility.h"

	"Src/Runtime/Utilities/algorithm_utility.h"
	"Src/Runtime/Utilities/Annotations.h"
	"Src/Runtime/Utilities/BitUtility.h"
	"Src/Runtime/Utilities/dense_hash_map.h"
	"Src/Runtime/Utilities/densehashtable.h"
	"Src/Runtime/Utilities/dynamic_array.h"
	"Src/Runtime/Utilities/EnumFlags.h"
	"Src/Runtime/Utilities/FileStripped.h"
	"Src/Runtime/Utilities/LinkedList.h"
	"Src/Runtime/Utilities/MemoryUtilities.h"
	"Src/Runtime/Utilities/MemoryUtilities.cpp"
	"Src/Runtime/Utilities/NonCopyable.h"
	"Src/Runtime/Utilities/ThreadedFlag.h"
	"Src/Runtime/Utilities/type_traits.h"
	"Src/Runtime/Utilities/TypeUtilities.h"
	"Src/Runtime/Utilities/UnionTuple.h"
	"Src/Runtime/Utilities/UniqueIDGenerator.h"
	"Src/Runtime/Utilities/UniqueIDGenerator.cpp"
	"Src/Runtime/Utilities/Utility.h"
	"Src/Runtime/Utilities/Word.h"
	"Src/Runtime/Utilities/ArrayUtility.h"
	)

macro(source_group_by_dir source_files)
	if(MSVC)
        set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
        foreach(sgbd_file ${${source_files}})
            string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC)
endmacro(source_group_by_dir)

FILE(GLOB_RECURSE project_headers *.h)
FILE(GLOB_RECURSE project_cpps *.cpp)
SET(all_files ../${project_headers} ../${project_cpps})
source_group_by_dir(all_files)

set(LIBRARY_OUTPUT_PATH $(PROJECT_BINARY_DIR)/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
include_directories(${ADDIT_INCLUDE_DIR})
ADD_DEFINITIONS(-DUSE_MONO -DNATIVEPARTICLESYSTEM_EXPORTS)
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

add_library(NativeParticleSystem SHARED ${LIB_INCLUDE} ${LIB_SRC} ${PLATFORM_INCLUDE} ${PLATFORM_SRC} ${OPENGLES_SRC})
if(MSVC)
	set_target_properties(NativeParticleSystem PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
	set_target_properties(NativeParticleSystem PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
	add_custom_command(TARGET NativeParticleSystem POST_BUILD
    COMMAND copy Build\\Debug\\NativeParticleSystem.dll ..\\..\\..\\Assets\\Plugins\\NativeParticleSystem.dll
    ) 
endif(MSVC)
#add_library(NativeParticleSystem SHARED ${LIB_SRC}) 

#set(LIB_SRC ../Src/Hello.c)
#set(APP_SRC ../Src/main.c)

#set(LIBRARY_OUTPUT_PATH $(PROJECT_BINARY_DIR)/lib)
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#add_library(NativeParticleSystem SHARED ${LIB_SRC} ${LIB_SRC_MONO})
#add_library(NativeParticleSystem SHARED &{all_files}) 

#add_library(libhello ${LIB_SRC})
#add_executable (hello ${APP_SRC})
# target_link_libraries(hello libhello)